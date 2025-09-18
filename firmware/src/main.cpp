#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <SPIFFS.h>
#include <Preferences.h>

// --- Configuração de Pinos ---
// Bombas (Relés)
const int PUMP_PINS[4] = {23, 22, 19, 18}; // Circulação, Filtragem, Borda, Aquecimento
const char* PUMP_NAMES[4] = {"Circulação", "Filtragem", "Borda", "Aquecimento"};

// LED Integrado (para status)
const int BUILTIN_LED_PIN = 2;

// Sensores
const int ONE_WIRE_BUS_PIN = 4;    // DS18B20
const int LDR_PIN = 34;            // LDR

// Iluminação RGB (LEDC/PWM)
const int RGB_PINS[3] = {25, 26, 27}; // R, G, B
const int RGB_CHANNELS[3] = {0, 1, 2}; // LEDC Channels

// --- Variáveis de Estado Globais ---
bool pumpStates[4] = {false, false, false, false};
float currentTemperature = -127.0;
int currentLuminosity = 0;
uint8_t currentColor[3] = {255, 0, 255}; // R, G, B - Roxo padrão

// --- Objetos de Hardware/Serviços ---
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
OneWire oneWire(ONE_WIRE_BUS_PIN);
DallasTemperature sensors(&oneWire);
Preferences preferences;

// --- Timers Não-Bloqueantes ---
unsigned long lastSensorReadTime = 0;
const long sensorReadInterval = 5000; // 5 segundos
unsigned long lastBroadcastTime = 0;
const long broadcastInterval = 2000; // 2 segundos

// --- Declarações de Funções ---
void setPumpState(int pumpId, bool state);
void setRgbColor(uint8_t r, uint8_t g, uint8_t b);
void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
void setupWiFi();
void setupWiFiAP();
String getMainPage();
String getConfigPage();
void updateSensors();
void broadcastFullState();
void parseHexColor(const char* hex, uint8_t& r, uint8_t& g, uint8_t& b);
void loadPumpStates();
void savePumpStates();


void setup() {
    Serial.begin(115200);
    Serial.println("\n\n🏛️ Quinta dos Britos - Pool Controller");

    // Inicializa LED de status
    pinMode(BUILTIN_LED_PIN, OUTPUT);
    digitalWrite(BUILTIN_LED_PIN, LOW);

    // Carrega estados das bombas da NVS
    loadPumpStates();

    // Inicializa relés das bombas e aplica estados restaurados
    for (int i = 0; i < 4; i++) {
        pinMode(PUMP_PINS[i], OUTPUT);
        digitalWrite(PUMP_PINS[i], pumpStates[i] ? HIGH : LOW);
    }

    // Inicializa sensores
    sensors.begin();

    // Inicializa iluminação RGB via LEDC
    for (int i = 0; i < 3; i++) {
        ledcSetup(RGB_CHANNELS[i], 5000, 8); // Canal, Frequência, Resolução
        ledcAttachPin(RGB_PINS[i], RGB_CHANNELS[i]);
    }
    setRgbColor(currentColor[0], currentColor[1], currentColor[2]); // Define cor inicial

    setupWiFi();

    ws.onEvent(onWebSocketEvent);
    server.addHandler(&ws);

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/html", getMainPage());
    });

    server.onNotFound([](AsyncWebServerRequest *request) {
        request->redirect("http://192.168.4.1/");
    });

    server.begin();
    Serial.println("✅ Servidor iniciado em http://192.168.4.1");
}

void loop() {
    ws.cleanupClients();

    unsigned long currentTime = millis();

    if (currentTime - lastSensorReadTime >= sensorReadInterval) {
        lastSensorReadTime = currentTime;
        updateSensors();
    }

    if (currentTime - lastBroadcastTime >= broadcastInterval) {
        lastBroadcastTime = currentTime;
        broadcastFullState();
        // Pisca o LED para indicar atividade
        digitalWrite(BUILTIN_LED_PIN, !digitalRead(BUILTIN_LED_PIN));
    }
}

// --- Funções de Controle ---

void setPumpState(int pumpId, bool state) {
    if (pumpId < 0 || pumpId >= 4) return;
    pumpStates[pumpId] = state;
    digitalWrite(PUMP_PINS[pumpId], state ? HIGH : LOW);
    Serial.printf("Bomba %d (%s) -> %s\n", pumpId, PUMP_NAMES[pumpId], state ? "ON" : "OFF");
    savePumpStates(); // Salva estados na NVS
    broadcastFullState();
}

void setRgbColor(uint8_t r, uint8_t g, uint8_t b) {
    currentColor[0] = r;
    currentColor[1] = g;
    currentColor[2] = b;
    for (int i = 0; i < 3; i++) {
        ledcWrite(RGB_CHANNELS[i], currentColor[i]);
    }
    Serial.printf("RGB Cor -> R:%d, G:%d, B:%d\n", r, g, b);
    broadcastFullState();
}

void updateSensors() {
    // Temperatura
    sensors.requestTemperatures();
    float tempC = sensors.getTempCByIndex(0);
    if (tempC != DEVICE_DISCONNECTED_C) {
        currentTemperature = tempC;
        Serial.printf("🌡️ Temperatura: %.2f°C\n", currentTemperature);
    } else {
        Serial.println("❌ Erro ao ler sensor de temperatura!");
    }

    // Luminosidade
    int rawLDR = analogRead(LDR_PIN);
    currentLuminosity = map(rawLDR, 0, 4095, 100, 0); // Invertido: mais luz = menor valor
    Serial.printf("☀️ Luminosidade: %d%%\n", currentLuminosity);
}

// --- Funções de Rede ---

void broadcastFullState() {
    JsonDocument doc;
    doc["action"] = "full_state";

    JsonArray pump_states = doc.createNestedArray("pumps");
    for (int i = 0; i < 4; i++) {
        pump_states.add(pumpStates[i]);
    }

    JsonObject sensors_data = doc.createNestedObject("sensors");
    sensors_data["temperature"] = currentTemperature;
    sensors_data["luminosity"] = currentLuminosity;

    JsonObject rgb = doc.createNestedObject("rgb");
    rgb["r"] = currentColor[0];
    rgb["g"] = currentColor[1];
    rgb["b"] = currentColor[2];

    String output;
    serializeJson(doc, output);
    ws.textAll(output);
}

void setupWiFi() {
    // Tenta carregar credenciais salvas da NVS
    preferences.begin("wifi-creds", true);
    String savedSSID = preferences.getString("wifi_ssid", "");
    String savedPass = preferences.getString("wifi_pass", "");
    preferences.end();
    
    if (savedSSID.length() > 0) {
        Serial.print("🔌 Conectando à rede WiFi salva: ");
        Serial.println(savedSSID);
        
        WiFi.mode(WIFI_STA);
        WiFi.begin(savedSSID.c_str(), savedPass.c_str());
        
        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 20) {
            delay(500);
            Serial.print(".");
            attempts++;
        }
        
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("\n✅ Conectado à rede WiFi!");
            Serial.print("📡 IP Address: ");
            Serial.println(WiFi.localIP());
            Serial.print("🌐 Gateway: ");
            Serial.println(WiFi.gatewayIP());
            Serial.print("📶 Signal Strength: ");
            Serial.print(WiFi.RSSI());
            Serial.println(" dBm");
            return;
        } else {
            Serial.println("\n❌ Falha ao conectar à rede WiFi salva.");
        }
    } else {
        Serial.println("📝 Nenhuma credencial WiFi salva encontrada.");
    }
    
    // Se chegou aqui, falhou ou não tem credenciais - entra em modo AP
    Serial.println("🔧 Iniciando modo AP para configuração...");
    setupWiFiAP();
}

void setupWiFiAP() {
    const char* ssid = "Quinta-dos-Britos-Config";
    WiFi.softAP(ssid, "12345678");
    Serial.printf("📡 AP Iniciado. SSID: %s\n", ssid);
    Serial.printf("💻 IP: %s\n", WiFi.softAPIP().toString().c_str());
    
    // Inicializa SPIFFS para servir arquivos
    if (!SPIFFS.begin(true)) {
        Serial.println("❌ Erro ao montar SPIFFS");
        return;
    }
    
    // Portal cativo simples - redireciona todas as requisições
    
    // Rota principal - página de configuração
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/html", getConfigPage());
    });
    
    // API para escanear redes WiFi
    server.on("/api/scanwifi", HTTP_GET, [](AsyncWebServerRequest *request) {
        int n = WiFi.scanNetworks();
        JsonDocument doc;
        JsonArray networks = doc.to<JsonArray>();
        
        for (int i = 0; i < n; i++) {
            JsonObject network = networks.add<JsonObject>();
            network["ssid"] = WiFi.SSID(i);
            network["rssi"] = WiFi.RSSI(i);
            network["encryption"] = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "open" : "secured";
        }
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });
    
    // API para salvar credenciais WiFi
    server.on("/api/savewifi", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (request->hasParam("ssid") && request->hasParam("password")) {
            String ssid = request->getParam("ssid")->value();
            String password = request->getParam("password")->value();
            
            // Salva credenciais na NVS
            preferences.begin("wifi-creds", false);
            preferences.putString("wifi_ssid", ssid);
            preferences.putString("wifi_pass", password);
            preferences.end();
            
            Serial.printf("💾 Credenciais salvas: %s\n", ssid.c_str());
            request->send(200, "text/plain", "Credenciais salvas! Reiniciando...");
            
            delay(1000);
            ESP.restart();
        } else {
            request->send(400, "text/plain", "SSID e senha são obrigatórios");
        }
    });
    
    // Captive portal - redireciona tudo para a página de configuração
    server.onNotFound([](AsyncWebServerRequest *request) {
        request->redirect("http://192.168.4.1/");
    });
    
    server.begin();
    Serial.println("🌐 Servidor de configuração iniciado");
}

void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_CONNECT) {
        Serial.printf("Cliente #%u conectado.\n", client->id());
        broadcastFullState(); // Envia estado atual ao novo cliente
    } else if (type == WS_EVT_DISCONNECT) {
        Serial.printf("Cliente #%u desconectado.\n", client->id());
    } else if (type == WS_EVT_DATA) {
        JsonDocument doc;
        if (deserializeJson(doc, data, len) == DeserializationError::Ok) {
            const char* action = doc["action"];
            if (strcmp(action, "set_pump") == 0) {
                setPumpState(doc["pump_id"], doc["state"]);
            } else if (strcmp(action, "set_rgb") == 0) {
                const char* hexColor = doc["color"]; // ex: "#RRGGBB"
                uint8_t r, g, b;
                parseHexColor(hexColor, r, g, b);
                setRgbColor(r, g, b);
            }
        }
    }
}

// --- Funções Utilitárias ---
void parseHexColor(const char* hex, uint8_t& r, uint8_t& g, uint8_t& b) {
    if (hex && hex[0] == '#') {
        long color = strtol(&hex[1], NULL, 16);
        r = (color >> 16) & 0xFF;
        g = (color >> 8) & 0xFF;
        b = color & 0xFF;
    }
}

void savePumpStates() {
    preferences.begin("pump-states", false);
    preferences.putBytes("states", pumpStates, sizeof(pumpStates));
    preferences.end();
    Serial.println("💾 Estados das bombas salvos na NVS");
}

void loadPumpStates() {
    preferences.begin("pump-states", true);
    if (preferences.isKey("states")) {
        preferences.getBytes("states", pumpStates, sizeof(pumpStates));
        Serial.println("🔄 Estados das bombas carregados da NVS");
        for (int i = 0; i < 4; i++) {
            Serial.printf("   Bomba %d (%s): %s\n", i, PUMP_NAMES[i], pumpStates[i] ? "ON" : "OFF");
        }
    } else {
        Serial.println("📝 Nenhum estado salvo encontrado, usando padrões");
        // Estados padrão já estão definidos na inicialização
        for (int i = 0; i < 4; i++) {
            pumpStates[i] = false;
        }
    }
    preferences.end();
}

String getConfigPage() {
    return String(R"(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Configuração WiFi - Quinta dos Britos</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body { 
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            display: flex;
            align-items: center;
            justify-content: center;
            padding: 20px;
        }
        .container {
            background: white;
            border-radius: 20px;
            box-shadow: 0 20px 40px rgba(0,0,0,0.1);
            padding: 40px;
            max-width: 500px;
            width: 100%;
        }
        .header {
            text-align: center;
            margin-bottom: 30px;
        }
        .header h1 {
            color: #333;
            font-size: 28px;
            margin-bottom: 10px;
        }
        .header p {
            color: #666;
            font-size: 16px;
        }
        .form-group {
            margin-bottom: 20px;
        }
        label {
            display: block;
            margin-bottom: 8px;
            color: #333;
            font-weight: 500;
        }
        select, input {
            width: 100%;
            padding: 12px 16px;
            border: 2px solid #e1e5e9;
            border-radius: 10px;
            font-size: 16px;
            transition: border-color 0.3s;
        }
        select:focus, input:focus {
            outline: none;
            border-color: #667eea;
        }
        .btn {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            border: none;
            padding: 12px 24px;
            border-radius: 10px;
            font-size: 16px;
            font-weight: 500;
            cursor: pointer;
            transition: transform 0.2s;
            width: 100%;
        }
        .btn:hover {
            transform: translateY(-2px);
        }
        .btn:disabled {
            opacity: 0.6;
            cursor: not-allowed;
            transform: none;
        }
        .status {
            margin-top: 20px;
            padding: 12px;
            border-radius: 10px;
            text-align: center;
            font-weight: 500;
        }
        .status.success {
            background: #d4edda;
            color: #155724;
            border: 1px solid #c3e6cb;
        }
        .status.error {
            background: #f8d7da;
            color: #721c24;
            border: 1px solid #f5c6cb;
        }
        .status.info {
            background: #d1ecf1;
            color: #0c5460;
            border: 1px solid #bee5eb;
        }
        .scan-btn {
            background: #28a745;
            margin-bottom: 20px;
        }
        .loading {
            display: none;
            text-align: center;
            margin: 20px 0;
        }
        .spinner {
            border: 3px solid #f3f3f3;
            border-top: 3px solid #667eea;
            border-radius: 50%;
            width: 30px;
            height: 30px;
            animation: spin 1s linear infinite;
            margin: 0 auto 10px;
        }
        @keyframes spin {
            0% { transform: rotate(0deg); }
            100% { transform: rotate(360deg); }
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>🏛️ Quinta dos Britos</h1>
            <p>Configuração da Rede WiFi</p>
        </div>
        
        <form id="wifiForm">
            <div class="form-group">
                <label for="ssid">Rede WiFi:</label>
                <select id="ssid" name="ssid" required>
                    <option value="">Clique em 'Escanear' para ver redes disponíveis</option>
                </select>
            </div>
            
            <div class="form-group">
                <label for="password">Senha:</label>
                <input type="password" id="password" name="password" placeholder="Digite a senha da rede">
            </div>
            
            <button type="button" id="scanBtn" class="btn scan-btn">🔍 Escanear Redes</button>
            <button type="submit" id="saveBtn" class="btn" disabled>💾 Salvar e Conectar</button>
        </form>
        
        <div id="status"></div>
        <div id="loading" class="loading">
            <div class="spinner"></div>
            <p>Configurando...</p>
        </div>
    </div>

    <script>
        const ssidSelect = document.getElementById('ssid');
        const passwordInput = document.getElementById('password');
        const scanBtn = document.getElementById('scanBtn');
        const saveBtn = document.getElementById('saveBtn');
        const form = document.getElementById('wifiForm');
        const status = document.getElementById('status');
        const loading = document.getElementById('loading');

        function showStatus(message, type) {
            status.innerHTML = message;
            status.className = 'status ' + type;
        }

        function showLoading(show) {
            loading.style.display = show ? 'block' : 'none';
            scanBtn.disabled = show;
            saveBtn.disabled = show;
        }

        async function scanNetworks() {
            showLoading(true);
            showStatus('Escanando redes WiFi...', 'info');
            
            try {
                const response = await fetch('/api/scanwifi');
                const networks = await response.json();
                
                ssidSelect.innerHTML = '<option value="">Selecione uma rede</option>';
                
                networks.forEach(network => {
                    const option = document.createElement('option');
                    option.value = network.ssid;
                    option.textContent = `${network.ssid} (${network.rssi} dBm) ${network.encryption === 'open' ? '🔓' : '🔒'}`;
                    ssidSelect.appendChild(option);
                });
                
                showStatus(`${networks.length} redes encontradas`, 'success');
                saveBtn.disabled = false;
            } catch (error) {
                showStatus('Erro ao escanear redes: ' + error.message, 'error');
            } finally {
                showLoading(false);
            }
        }

        async function saveWiFi(event) {
            event.preventDefault();
            
            const ssid = ssidSelect.value;
            const password = passwordInput.value;
            
            if (!ssid) {
                showStatus('Por favor, selecione uma rede WiFi', 'error');
                return;
            }
            
            showLoading(true);
            showStatus('Salvando configuração...', 'info');
            
            try {
                const formData = new FormData();
                formData.append('ssid', ssid);
                formData.append('password', password);
                
                const response = await fetch('/api/savewifi', {
                    method: 'POST',
                    body: formData
                });
                
                if (response.ok) {
                    showStatus('✅ Configuração salva! O dispositivo está reiniciando...', 'success');
                    setTimeout(() => {
                        showStatus('🔄 Reiniciando... Aguarde alguns segundos e tente conectar à sua rede WiFi.', 'info');
                    }, 2000);
                } else {
                    const error = await response.text();
                    showStatus('Erro: ' + error, 'error');
                }
            } catch (error) {
                showStatus('Erro ao salvar: ' + error.message, 'error');
            } finally {
                showLoading(false);
            }
        }

        scanBtn.addEventListener('click', scanNetworks);
        form.addEventListener('submit', saveWiFi);
        
        // Escanear automaticamente ao carregar a página
        window.addEventListener('load', scanNetworks);
    </script>
</body>
</html>
)");
}

// --- Interface HTML ---
String getMainPage() {
    return String(R"(
<!DOCTYPE html>
<html lang="pt-BR" class="dark">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Quinta dos Britos - Piscina</title>
    <script src="https://cdn.tailwindcss.com"></script>
    <style>
        :root { --main-hue: 231; }
        body { 
            background-color: hsl(var(--main-hue), 15%, 15%);
            background-image: radial-gradient(circle at 1px 1px, hsl(var(--main-hue), 15%, 20%) 1px, transparent 0);
            background-size: 20px 20px;
            transition: --main-hue 0.5s ease;
        }
        .pump-card.pump-active {
            box-shadow: 0 0 20px 5px hsla(var(--main-hue), 90%, 60%, 0.5);
            border-color: hsl(var(--main-hue), 90%, 60%);
        }
    </style>
</head>
<body class="text-gray-200 font-sans">
    <div class="container mx-auto p-4 max-w-2xl">
        <header class="text-center py-6">
            <h1 class="text-4xl md:text-5xl font-bold tracking-tight">🏛️ Quinta dos Britos</h1>
            <p class="text-xl text-gray-400 mt-2">Sistema de Automação da Piscina</p>
        </header>
        <main>
            <section class="grid grid-cols-2 gap-4 mb-6 text-center">
                <div class="bg-gray-800/50 backdrop-blur-sm p-4 rounded-lg">
                    <h3 class="font-semibold text-lg">🌡️ Temperatura</h3>
                    <p class="text-3xl font-mono" id="temp-display">--.- °C</p>
                </div>
                <div class="bg-gray-800/50 backdrop-blur-sm p-4 rounded-lg">
                    <h3 class="font-semibold text-lg">☀️ Luminosidade</h3>
                    <p class="text-3xl font-mono" id="lumi-display">-- %</p>
                </div>
            </section>
            <section class="grid grid-cols-2 md:grid-cols-4 gap-4 mb-6">
    )") +
    [&]() {
        String cards = "";
        for (int i = 0; i < 4; ++i) {
            cards += R"(<div id="card)" + String(i) + R"(" class="pump-card bg-gray-800/50 backdrop-blur-sm p-4 rounded-lg text-center border-2 border-transparent transition-all duration-300">
                    <h3 class="font-bold text-lg mb-2">)" + PUMP_NAMES[i] + R"(</h3>
                    <input type="checkbox" id="pump)" + String(i) + R"(" class="toggle-checkbox hidden">
                    <label for="pump)" + String(i) + R"(" class="cursor-pointer inline-block w-14 h-8 bg-gray-600 rounded-full p-1 transition-colors duration-300">
                        <span class="inline-block w-6 h-6 bg-white rounded-full shadow-md transform transition-transform duration-300"></span>
                    </label>
                </div>)";
        }
        return cards;
    }() +
    String(R"(
            </section>
            <section class="bg-gray-800/50 backdrop-blur-sm p-4 rounded-lg">
                <h3 class="font-semibold text-lg mb-2 text-center">🎨 Iluminação RGB</h3>
                <div class="flex justify-center items-center">
                    <input type="color" id="colorPicker" value="#FF00FF" class="w-24 h-12 p-1 bg-gray-700 rounded-md cursor-pointer">
                </div>
            </section>
            <footer class="text-center mt-6">
                <p id="connectionStatus" class="font-mono text-sm text-red-500">🔴 Desconectado</p>
            </footer>
        </main>
    </div>
    <script>
        const ws = new WebSocket(`ws://${window.location.host}/ws`);

        function hexToHsl(hex) {
            const result = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(hex);
            let r = parseInt(result[1], 16) / 255, g = parseInt(result[2], 16) / 255, b = parseInt(result[3], 16) / 255;
            const max = Math.max(r, g, b), min = Math.min(r, g, b);
            let h, s, l = (max + min) / 2;
            if (max === min) { h = s = 0; }
            else {
                const d = max - min;
                s = l > 0.5 ? d / (2 - max - min) : d / (max + min);
                switch (max) {
                    case r: h = (g - b) / d + (g < b ? 6 : 0); break;
                    case g: h = (b - r) / d + 2; break;
                    case b: h = (r - g) / d + 4; break;
                }
                h /= 6;
            }
            return { h: Math.round(h * 360), s: Math.round(s * 100), l: Math.round(l * 100) };
        }

        ws.onopen = () => document.getElementById('connectionStatus').textContent = '🟢 Conectado';
        ws.onclose = () => document.getElementById('connectionStatus').textContent = '🔴 Desconectado';

        ws.onmessage = (event) => {
            const state = JSON.parse(event.data);
            if (state.action !== 'full_state') return;

            state.pumps.forEach((isOn, i) => {
                document.getElementById(`pump${i}`).checked = isOn;
                document.getElementById(`card${i}`).classList.toggle('pump-active', isOn);
            });

            document.getElementById('temp-display').textContent = `${state.sensors.temperature.toFixed(1)} °C`;
            document.getElementById('lumi-display').textContent = `${state.sensors.luminosity} %`;
            
            const hexColor = `#${state.rgb.r.toString(16).padStart(2, '0')}${state.rgb.g.toString(16).padStart(2, '0')}${state.rgb.b.toString(16).padStart(2, '0')}`;
            document.getElementById('colorPicker').value = hexColor;

            const hsl = hexToHsl(hexColor);
            document.documentElement.style.setProperty('--main-hue', hsl.h);
        };

        for(let i=0; i<4; i++) {
            document.getElementById(`pump${i}`).addEventListener('change', (e) => {
                ws.send(JSON.stringify({ action: 'set_pump', pump_id: i, state: e.target.checked }));
            });
        }

        document.getElementById('colorPicker').addEventListener('input', (e) => {
            ws.send(JSON.stringify({ action: 'set_rgb', color: e.target.value }));
        });

        const style = document.createElement('style');
        style.innerHTML = `.toggle-checkbox:checked + label span { transform: translateX(1.5rem); } .toggle-checkbox:checked + label { background-color: hsl(var(--main-hue), 80%, 60%); }`;
        document.head.appendChild(style);
    </script>
</body>
</html>
    )");
}
