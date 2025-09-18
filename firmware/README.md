# ESP32 Pool Controller Firmware

Este diretório contém o firmware para o ESP32 que controla o sistema de automação da piscina.

## Pré-requisitos

1. **PlatformIO**: Instale o PlatformIO Core ou use a extensão do VS Code
2. **esptool**: Para upload direto via terminal como solicitado

### Instalação do PlatformIO (via pip)
```bash
pip install platformio
```

### Instalação do esptool
```bash
pip install esptool
```

## Compilação e Upload

### Método 1: PlatformIO (Recomendado para desenvolvimento)
```bash
cd firmware
pio run                    # Compilar
pio run --target upload    # Upload para ESP32
pio run --target monitor   # Monitor serial
```

### Método 2: esptool direto (conforme solicitado)
```bash
cd firmware
pio run                    # Compilar primeiro
esptool.py --chip esp32 --port /dev/cu.usbserial* --baud 921600 write_flash -z 0x1000 .pio/build/esp32dev/bootloader.bin 0x8000 .pio/build/esp32dev/partitions.bin 0xe000 ~/.platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin 0x10000 .pio/build/esp32dev/firmware.bin
```

## Configuração de Hardware

### Conexões dos Relés
- **Bomba 0 (Circulação)**: GPIO 23
- **Bomba 1 (Filtragem)**: GPIO 22  
- **Bomba 2 (Aquecimento)**: GPIO 21
- **Bomba 3 (Borda)**: GPIO 19

### Configuração WiFi
- **SSID**: ESP32-Pool-Controller
- **Senha**: poolcontrol123
- **IP**: 192.168.4.1

## Teste do WebSocket

Você pode testar o controle das bombas usando qualquer cliente WebSocket conectando em:
`ws://192.168.4.1/ws`

### Comandos JSON
```json
{
  "action": "set_pump",
  "pump_id": 0,
  "state": true
}
```

- `pump_id`: 0-3 (Circulação, Filtragem, Aquecimento, Borda)
- `state`: true (ligar) ou false (desligar)

### Resposta esperada
```json
{
  "action": "pump_status",
  "pump_id": 0,
  "state": true,
  "success": true
}
```

## Monitoramento

Para ver os logs em tempo real:
```bash
pio run --target monitor
```

Ou com esptool:
```bash
python -m serial.tools.miniterm /dev/cu.usbserial* 115200
```

## Próximas Implementações

- [ ] Sensores de temperatura e luminosidade
- [ ] Controle RGB LED
- [ ] Sistema de agendamento
- [ ] Integração com Tuya IoT
- [ ] Persistência de estado (NVS)
