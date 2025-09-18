# 🏛️ Quinta dos Britos - Sistema de Automação de Piscina

![ESP32](https://img.shields.io/badge/ESP32-blue?style=for-the-badge&logo=espressif)
![PlatformIO](https://img.shields.io/badge/PlatformIO-orange?style=for-the-badge&logo=platformio)
![Next.js](https://img.shields.io/badge/Next.js-black?style=for-the-badge&logo=next.js)
![Shadcn/UI](https://img.shields.io/badge/Shadcn/UI-black?style=for-the-badge)

Sistema completo de IoT para controle e automação inteligente da piscina da Quinta dos Britos. Desenvolvido com ESP32, PlatformIO e uma interface web moderna.

## ✨ Visão Geral

Este projeto visa automatizar completamente o gerenciamento dos equipamentos da piscina, oferecendo:
- **Controle Remoto**: Interface web moderna e intuitiva para controle de qualquer dispositivo.
- **Automação Inteligente**: Lógica baseada em sensores para otimizar o funcionamento e economizar energia.
- **Integração com IoT**: Futura integração com a plataforma Tuya para controle por voz com Alexa e Google Assistant.

## 🚀 Status Atual do Projeto (MVP)

Atualmente, o **firmware base está 100% funcional** e versionado no Git.

### ✅ Funcionalidades Implementadas:

- **Controle Manual de 4 Bombas**:
  - 💧 Circulação (GPIO 23)
  - 🔧 Filtragem (GPIO 22)
  - 🔥 Aquecimento (GPIO 2 - LED da placa)
  - 🌊 Borda Infinita (GPIO 19)
- **Interface Web Local**:
  - Acessível via WiFi AP (`Quinta-dos-Britos`) no IP `192.168.4.1`.
  - **Portal Captivo**: A interface abre automaticamente ao conectar na rede.
  - **Design Responsivo**: Interface elegante com tema "Quinta dos Britos".
  - **Animações Visuais**: Feedback em tempo real do funcionamento das bombas.
- **Comunicação em Tempo Real**: WebSocket para controle instantâneo e feedback.

### 🎯 Próximos Passos (Roadmap):

1.  **Implementação de Sensores**: Leitura de temperatura (DS18B20) e luminosidade (LDR).
2.  **Lógica de Automação**: Acionamento automático das bombas com base nos sensores.
3.  **Controle de Iluminação RGB**: Adicionar controle de cores para a iluminação da piscina.
4.  **Integração com Tuya**: Conectar o sistema à nuvem para controle por voz e acesso remoto.

## 🛠️ Pilha de Tecnologia (Tech Stack)

- **Hardware**:
  - Microcontrolador: **ESP32 DevKit V1**
  - Módulos: 4x Relés para as bombas
- **Firmware**:
  - Framework: **Arduino**
  - Ambiente: **PlatformIO**
  - Bibliotecas: `ESPAsyncWebServer`, `ArduinoJson`
- **Frontend (Planejado)**:
  - Framework: **Next.js 14**
  - Linguagem: **TypeScript**
  - UI: **Shadcn/UI** e **Tailwind CSS**

## ⚙️ Setup e Instalação

### Pré-requisitos

- [Python 3.9+](https://www.python.org/downloads/)
- [PlatformIO Core](https://docs.platformio.org/en/latest/core/installation/index.html) (`pip install -U platformio`)
- [Git](https://git-scm.com/downloads)

### Firmware

1.  **Clone o repositório:**
    ```bash
    git clone https://github.com/felipebrito/quinta_piscina.git
    cd quinta_piscina/firmware
    ```
2.  **Conecte o ESP32**: Conecte sua placa ESP32 via USB. O sistema tentará detectar a porta automaticamente.
3.  **Compile e Faça o Upload**:
    ```bash
    # Compila, faz o upload e inicia o monitor serial
    pio run --target upload && pio run --target monitor
    ```

## 🔄 Como Recomeçar o Projeto do Zero

Se em algum momento você desejar apagar todo o progresso e recomeçar este projeto do início com meu auxílio, use o seguinte prompt:

> **Prompt para Recomeçar:**
>
> "Vamos recomeçar o projeto Quinta dos Britos - Piscina do zero. Apague todos os arquivos existentes (exceto o `.git`), e vamos iniciar com a discussão da ideia para gerar um novo PRD, exatamente como fizemos da primeira vez."

---
*Este projeto está sendo desenvolvido com o auxílio de um assistente de IA.*
