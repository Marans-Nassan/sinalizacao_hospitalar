# Sistema de Sinalização Hospitalar Interativa  

## Descrição  
Projeto de sinalização para ambientes hospitalares utilizando Raspberry Pi Pico W (RP2040). O sistema integra:  
- **Sinalização visual** com LEDs (verde, vermelho e azul).  
- **Alertas sonoros** via buzzer (PWM).  
- **Display OLED** para debug em tempo real.  
- **Matriz de LEDs WS2812B** para apresentar o estado do plantão/equipe.  
- **Controle via botões e joystick** para interação rápida.  

Funcionalidades adaptadas para cenários médicos, como chamadas de enfermagem e alertas de emergência.  

---

## Componentes e Funcionalidades  
### Elementos Principais  
| Componente       | GPIO/Pino    | Função Hospitalar                               |   
| LED Verde        | GPIO 11      | Status normal. (Em atividade)                   |  
| LED Vermelho     | GPIO 13      | Emergência. (prioridade máxima)                 |  
| LED Azul         | GPIO 12      | Chamada de enfermagem.                          |  
| Botão A          | GPIO 5       | Inicialização ou desligamento do Status normal. |  
| Botão B          | GPIO 6       | Alerta de necessidade médica.                   |  
| Botão J          | GPIO 22      | Estado Crítico! Alerta Sonoro.                  |  
| Buzzer           | GPIO 21      | Alerta sonoro de prioridade.                    |  
| Matriz de LEDs   | GPIO 7       | Códigos visuais (ex: ⚕️, 🔴, 🔵)               |  
| Joystick (VRX/VRY)| ADC0/ADC1   | Ferramenta para verificar o funcionamento.      |  

### Operação  
1. **Modo Rotina (Botão A)**:  
   - Aciona LED verde e matriz de LEDs com padrão azul.    

2. **Modo Necessidade (Botão B)**:  
   - Ativa LED azul e matriz de LEDS com padrão da cruz vermelha.  

3. **Ativar Alarme. (ESTADO CRÍTICO)(Botão J)**:  
   - Ativa o buzzer e alterna LED vermelho como confirmação. (Unindo todas as cores e incorrendo na luz branca)

4. **Joystick**:  
   - Realização de debug em tempo real para garantir que o equipamento esta funcionando.  

---

## Instalação  
1. **Hardware**:  
Compile o código utilizando CMake e o SDK do Pico.

## Dependências:

Biblioteca ssd1306 para OLED.

Driver PIO ws2812.pio para matriz de LEDs.

## Compilação:

bash
mkdir build && cd build  
cmake -DPICO_SDK_PATH=/caminho/do/sdk ..  
make  

## Autor
Hugo Martins Santana (TIC370101267)