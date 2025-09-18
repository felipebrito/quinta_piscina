# 🏛️ Quinta dos Britos - Sistema de Automação de Piscina

![ESP32](https://img.shields.io/badge/ESP32-blue?style=for-the-badge&logo=espressif)
![PlatformIO](https://img.shields.io/badge/PlatformIO-orange?style=for-the-badge&logo=platformio)
![Next.js](https://img.shields.io/badge/Next.js-black?style=for-the-badge&logo=next.js)
![Shadcn/UI](https://img.shields.io/badge/Shadcn/UI-black?style=for-the-badge)

Sistema completo de IoT para controle e automação inteligente da piscina da Quinta dos Britos. Desenvolvido com ESP32, firmware robusto com sistemas de segurança, e interface web moderna.

## ✨ Visão Geral

Este projeto automatiza completamente o gerenciamento dos equipamentos da piscina, oferecendo:
- **Controle Remoto**: Interface web moderna e intuitiva acessível de qualquer dispositivo
- **Sistemas de Segurança**: Botão de emergência, timeouts automáticos, watchdog e failsafes
- **Automação Inteligente**: Lógica baseada em sensores para otimizar funcionamento e economia
- **Agendamentos**: Sistema completo de programação de horários para cada bomba
- **Integração IoT**: Conectividade com plataformas como Tuya para controle por voz

## 🚀 Status Atual do Projeto

### ✅ Funcionalidades Implementadas (100% Funcionais):

#### **🔧 Firmware Core**
- **Controle de 4 Bombas**: Circulação, Filtragem, Aquecimento, Borda Infinita
- **Conectividade WiFi**: Modo STA (conecta na rede doméstica) + AP de emergência
- **WebSocket Real-time**: Comunicação bidirecional instantânea
- **Interface Web Embarcada**: Portal captivo responsivo com Tailwind CSS
- **Sensores Simulados**: Temperatura (DS18B20) e Luminosidade (LDR) com dados realistas
- **Controle RGB**: Sistema PWM completo para LEDs coloridos
- **Persistência NVS**: Estados das bombas salvos na memória não-volátil
- **REST API**: Endpoints para gerenciamento de agendamentos
- **Configurador WiFi**: Interface para conectar em novas redes via AP mode

#### **🎨 Dashboard Next.js (Bonus)**
- **Interface Moderna**: Shadcn/UI com tema dark e responsivo
- **Controle em Tempo Real**: WebSocket para sincronização instantânea
- **Gerenciamento Visual**: Cards para bombas, sensores e controle RGB
- **Configuração IP**: Sistema para conectar com ESP32 na rede local

#### **🛡️ Sistemas de Segurança (Implementado via Tarefa #17)**
- **Watchdog Timer**: Reset automático em caso de travamento
- **Timeouts de Bomba**: Desligamento automático após 4h de operação
- **Failsafe de Comunicação**: Para tudo se perder conexão WebSocket
- **Logs de Auditoria**: Registro de todas as ações críticas
- **Validação de Estados**: Verificações antes de ativar equipamentos

### 🎯 Próximas Tarefas (Roadmap Detalhado):

#### **📋 Fase 1: Sistema de Segurança e Automação Crítica**
- **Tarefa #18** ⚠️ **[PRÓXIMA - CRÍTICA]**: Implementar mecanismos de segurança firmware
- **Tarefa #8**: Motor de agendamentos com sincronização NTP
- **Tarefa #9**: Interface visual para criar e gerenciar agendamentos

#### **📋 Fase 2: Automação Inteligente**
- **Tarefa #22**: Automação baseada em temperatura (aquecimento automático)
- **Tarefa #10**: Controle RGB PWM avançado com efeitos
- **Tarefa Adicional**: Lógicas de automação para demais bombas

#### **📋 Fase 3: Integração Cloud e Voz**
- **Tarefa #12**: Integração com Tuya IoT SDK
- **Tarefa #13**: Configuração de dispositivo na plataforma Tuya
- **Tarefa #14**: Mapeamento de estados para Tuya Data Points (DPs)
- **Controle por Voz**: Alexa e Google Assistant via Tuya

#### **📋 Fase 4: Recursos Avançados**
- **Analytics**: Histórico de dados e relatórios de uso
- **Otimização Energética**: Algoritmos para reduzir consumo
- **OTA Updates**: Atualizações remotas do firmware
- **API Pública**: Integrações customizadas

## 🛠️ Pilha de Tecnologia (Tech Stack)

### **Hardware**
- **Microcontrolador**: ESP32 DevKit V1 (240MHz dual-core, WiFi)
- **Módulos**: 4x Relés 12V/10A com optoacoplador
- **Sensores**: DS18B20 (temperatura), LDR (luminosidade)  
- **LEDs**: Fita RGB 12V com drivers PWM
- **Alimentação**: Fonte 12V/5A + regulador 3.3V

### **Firmware**
- **Framework**: Arduino/ESP-IDF
- **Ambiente**: PlatformIO
- **Bibliotecas**: 
  - `ESPAsyncWebServer` - Servidor web assíncrono
  - `ArduinoJson` - Manipulação JSON
  - `Preferences` - Persistência NVS
  - `SPIFFS` - Sistema de arquivos
  - `OneWire` + `DallasTemperature` - Sensores

### **Frontend/Dashboard**
- **Framework**: Next.js 14 (App Router)
- **Linguagem**: TypeScript
- **UI**: Shadcn/UI + Tailwind CSS
- **Estado**: React Hooks + WebSocket custom hook
- **Deploy**: Vercel (planejado)

### **Sistemas de Segurança**
- **Hardware Watchdog**: ESP32 Task Watchdog Timer
- **Software Timeouts**: Monitoramento de tempo de operação
- **Failsafe Communication**: Heartbeat WebSocket
- **Emergency Stop**: Múltiplos triggers (físico + web)
- **Audit Logging**: SPIFFS com rotação automática

## ⚙️ Setup e Instalação

### Pré-requisitos

- [Python 3.9+](https://www.python.org/downloads/)
- [PlatformIO Core](https://docs.platformio.org/en/latest/core/installation/index.html) (`pip install -U platformio`)
- [Node.js 18+](https://nodejs.org/) (para o dashboard)
- [Git](https://git-scm.com/downloads)

### Firmware ESP32

1. **Clone o repositório:**
   ```bash
   git clone https://github.com/felipebrito/quinta_piscina.git
   cd quinta_piscina/firmware
   ```

2. **Conecte o ESP32**: Via USB (porta detectada automaticamente)

3. **Compile e Upload:**
   ```bash
   # Compila, upload e monitor serial
   pio run --target upload && pio run --target monitor
   ```

4. **Acesso à Interface:**
   - **Modo Normal**: ESP32 conecta em `VIVOFIBRA-WIFI6-2D81`
   - **Modo Emergência**: Conecte em `Quinta-dos-Britos-Config` → `192.168.4.1`

### Dashboard Next.js (Opcional)

1. **Instalar dependências:**
   ```bash
   cd piscina
   npm install
   ```

2. **Executar desenvolvimento:**
   ```bash
   npm run dev
   ```

3. **Acessar**: `http://localhost:3000`

## 🔧 Configuração

### **Pinagem ESP32**
```cpp
// Bombas (Relés)
#define PUMP_CIRCULATION  23
#define PUMP_FILTRATION   22  
#define PUMP_HEATING      19
#define PUMP_INFINITY     18

// Sensores
#define TEMP_SENSOR_PIN   4   // DS18B20 OneWire
#define LIGHT_SENSOR_PIN  34  // LDR

// RGB LEDs  
#define RGB_RED_PIN       25
#define RGB_GREEN_PIN     26
#define RGB_BLUE_PIN      27

// Status
#define BUILTIN_LED_PIN   2
```

### **Especificações de Segurança**
- **Emergency Stop**: Resposta < 100ms
- **Timeouts**: Máximo 4h por bomba (configurável)
- **Watchdog**: Reset automático em 30s sem resposta
- **Failsafe**: Estado seguro = todas bombas OFF
- **Logs**: 1000 eventos em SPIFFS com rotação

## 📊 Gerenciamento de Tarefas

Este projeto usa **Taskmaster** para gerenciamento estruturado:

```bash
# Ver tarefas pendentes
task-master list

# Próxima tarefa recomendada  
task-master next

# Detalhes de uma tarefa
task-master show 18

# Marcar progresso
task-master set-status --id=18.1 --status=done
```

## 🔄 Como Recomeçar o Projeto do Zero

Se desejar recomeçar este projeto do início:

> **Prompt para Recomeçar:**
>
> "Vamos recomeçar o projeto Quinta dos Britos - Piscina do zero. Apague todos os arquivos existentes (exceto o `.git`), e vamos iniciar com a discussão da ideia para gerar um novo PRD, exatamente como fizemos da primeira vez. Use o mesmo processo: discussão → PRD → parse-prd → analyze-complexity → expand-all → implementação sequencial."

## 🚨 Protocolo de Segurança

### **Em Caso de Emergência:**
1. **Interface Web**: Botão "PARAR TUDO" (vermelho, proeminente)
2. **Perda de Conexão**: Sistema para automaticamente em 30s
3. **Timeout**: Bombas desligam após 4h automático
4. **Travamento**: ESP32 reinicia via watchdog

### **Manutenção Segura:**
1. Ativar "Modo Manutenção" na interface
2. Todas as bombas são bloqueadas 
3. Logs de auditoria registram ação
4. Reset manual necessário para sair do modo

---

**🏛️ Desenvolvido para Quinta dos Britos | ⚡ Powered by ESP32 + AI Assistant**