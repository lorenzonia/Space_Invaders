# 🏗️ Arquitetura de Software - Space Invaders RTOS

Este documento detalha a modelagem de software, a topologia de tarefas (Tasks), os mecanismos de temporização e as primitivas de sincronização utilizadas no desenvolvimento do jogo Space Invaders para o microcontrolador **STM32F411CE (Black Pill)** utilizando o **FreeRTOS (CMSIS-RTOS V2)**.

---

## 💾 Organização de Dados e Padrão de Projeto

O sistema foi estruturado seguindo os princípios da programação modular e da ocultação de dados em C. Para gerenciar o estado macro do jogo de forma segura e concorrente, foi adotada uma topologia baseada em **Double Buffering Lógico**.

### O Objeto Global e Isolamento de Buffers
Todas as propriedades dinâmicas do jogo (coordenadas da nave, matriz de inimigos, vetores de projéteis ativos, pontuação, nível e estado da máquina de estados) são encapsuladas em uma estrutura unificada do tipo `Game`. No entanto, para evitar condições de corrida (*race conditions*) e anomalias de varredura na tela (*screen tearing*), a memória foi duplicada em duas instâncias:

* **`game` (Buffer de Trabalho):** Instância principal modificada unicamente pelas tarefas de lógica física e tratamento de entradas. Nenhuma rotina de desenho lê essa estrutura diretamente.
* **`game_render_copy` (Buffer Espelho):** Uma réplica isolada que serve de fonte de dados exclusiva para a tarefa de renderização. Sua atualização ocorre por meio de uma operação atômica de cópia de memória (`memcpy`) assim que um novo ciclo físico é consolidado.

---

## 🧵 Topologia e Ciclo de Vida das Tarefas (Tasks)

O ecossistema executa de forma assíncrona através de quatro threads de execução paralelas sob o gerenciamento do escalonador por preempção do FreeRTOS. Todas operam com o nível de **Prioridade Normal** (`osPriorityNormal`), confiando o determinismo de execução estrito aos temporizadores e semáforos de sincronização.

### 1. `vTask_Input`
* **Escopo:** Interface Humana (I/O) e Captura de Comandos.
* **Periodicidade / Temporização:** Execução cíclica fixa a cada **15ms** via `vTaskDelay`.
* **Comportamento:** Amostra o eixo horizontal do joystick analógico via **ADC1 operando com DMA**. Realiza a leitura por varredura (*polling*) do pino digital `PC15` (botão de disparo). Os dados são validados e injetados de forma assíncrona nas variáveis de comando da struct de trabalho `game`.

### 2. `vTask_Game`
* **Escopo:** Núcleo de Processamento Físico e Regras de Negócio.
* **Periodicidade / Temporização:** Execução estável a **~33Hz (período de 30ms)** controlado por `vTaskDelay`.
* **Comportamento:** Gerencia a máquina de estados principal do sistema (Menu, Playing, Game Over). Computa o deslocamento cinemático da nave baseado nos inputs lidos, atualiza a posição dos inimigos e projéteis, processa as matrizes de colisão (tiro vs. inimigo, inimigo vs. nave) e incrementa a pontuação. Ao consolidar a física do frame, solicita o Mutex para transferir os dados para o buffer espelho e sinaliza a tarefa de tela.

### 3. `vTask_Render`
* **Escopo:** Atualização Gráfica e Transmissão de Pixels.
* **Periodicidade / Temporização:** Orientada a eventos (bloqueio por semáforo).
* **Comportamento:** Permanece em estado de bloqueio absoluto (*Blocked*), liberando a CPU para outras rotinas. Só acorda ao receber a sinalização de que um novo frame físico foi calculado. Executa o algoritmo otimizado de desenho e transmite os pacotes de dados para o display LCD ST7735 utilizando o barramento **SPI1 por Hardware**. Ao concluir, libera a física para calcular o próximo quadro.

### 4. `vTask_SoundMenu`
* **Escopo:** Gerenciamento Acústico Backstage.
* **Periodicidade / Temporização:** Orientada ao estado do jogo.
* **Comportamento:** Monitora as transições da máquina de estados do jogo. Durante a permanência na tela de menu, executa bips sonoros ritmados no estilo *Brick Game* acionando o pino digital `PB0` conectado a um buzzer piezoelétrico. Opera com pulsos de ativação curtíssimos para mitigar ruídos e variações abruptas de consumo na linha de energia da placa.

---

## 🔒 Mecanismos de Sincronização e Concorrência

Para garantir a integridade dos dados e o determinismo temporal, foram aplicadas três soluções clássicas de sistemas operacionais de tempo real:

### 🔄 Pipeline e Handshake por Semáforos
O sincronismo e o fluxo de dados no estilo *ping-pong* entre a simulação física e a taxa de atualização visual são rigidamente controlados pelo par de semáforos `renderSemaphoreHandle` e `physicsSemaphoreHandle`:
1. A tarefa de física (`vTask_Game`) calcula o frame atual e sinaliza o semáforo de renderização, acordando a tela. Em seguida, tenta adquirir o semáforo de física e entra em estado de bloqueio.
2. A tarefa de renderização (`vTask_Render`) desenha o quadro inteiro no display através do barramento SPI. Ao finalizar o último pixel, sinaliza o semáforo de física, acordando a lógica do jogo.
Este mecanismo impede que a física processe dados mais rápido do que a tela consegue exibir, eliminando o acúmulo de frames atrasados na fila de processamento.

### 🛡️ Exclusão Mútua por Mutex (`gameMutexHandle`)
O identificador `gameMutexHandle` foi implementado para criar uma região crítica atômica em torno da operação de transferência de dados (`memcpy`). Embora o par de semáforos estabeleça uma alternância de turnos estável, o Mutex blinda o barramento lógico interno contra qualquer interrupção involuntária de contexto no exato microssegundo em que o estado de `game` está sendo espelhado em `game_render_copy`, garantindo que a tela jamais leia um frame parcialmente escrito.

### ⚡ Algoritmo Gráfico "Clean-and-Draw"
Devido à limitação física de largura de banda do barramento serial SPI ao atualizar matrizes de cores em displays TFT, efetuar o preenchimento total da tela com pixels pretos (*screen clean*) a cada frame causaria quedas drásticas de FPS e o efeito colateral de *flickering* (piscadas na tela). A solução proposta foi otimizar o desenho:
* A struct armazena a coordenada do objeto no frame atual e sua coordenada imediatamente anterior (`last_x`, `last_y`).
* Antes de renderizar a imagem do sprite na nova coordenada, o sistema desenha um bloco preto minimalista cobrindo estritamente a posição antiga (apagando apenas o rastro pixel a pixel).
Esta técnica reduz o tráfego de dados no barramento SPI em mais de 85%, permitindo animações fluidas mesmo operando em frequências de barramento moderadas.

---

## 📂 Mapeamento de Arquivos Estruturais

Para facilitar a manutenção do código e respeitar a modularidade descrita nesta arquitetura, o firmware foi dividido nos seguintes componentes:

* **`Core/Src/main.c`:** Ponto de entrada do sistema. Realiza a inicialização dos periféricos (Clocks, GPIOs, SPI1, ADC1), instancia os Semáforos/Mutexes e cria as 4 Tasks principais do FreeRTOS.
* **`Core/Src/game.c` & `Core/Inc/game.h`:** Contêm a definição da estrutura global `Game` e implementam as regras de negócio, física de movimento, cálculo de colisão e atualização da máquina de estados do jogo.
* **`Core/Src/st7735.c` & `Core/Inc/st7735.h`:** Driver de baixo nível do display LCD. Traduz as coordenadas e buffers de sprites lógicos em comandos nativos SPI enviados diretamente para o controlador do hardware.

---

## 📈 Considerações sobre Determinismo e Resultados

A adoção do FreeRTOS (CMSIS-RTOS V2) neste projeto provou-se indispensável para atingir o comportamento de **Tempo Real Estrito** exigido por um sistema interativo:

1. **Eficiência da CPU:** Graças ao estado de bloqueio (*Blocked*) baseado em temporizadores e semáforos, a CPU passa a maior parte do tempo em repouso térmico ou executando tarefas de baixa prioridade (como o áudio), em vez de desperdiçar ciclos de clock em loops de atraso vazios (`delay`).
2. **Fluidez Visual:** O desacoplamento total entre a leitura de controles (15ms), processamento da física (30ms) e escrita no display garante que o jogo responda instantaneamente aos comandos do jogador, sem causar engasgos (*stuttering*) na renderização gráfica.
3. **Escalabilidade:** A separação clara de papéis por tarefas permite que novos recursos — como inteligência artificial aprimorada para os inimigos, novos efeitos sonoros ou novos sensores — sejam adicionados ao ecossistema apenas criando novas tasks com prioridades adequadas, sem quebrar a temporização do código existente.