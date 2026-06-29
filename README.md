# 🚀 Space Invaders STM32 (FreeRTOS)

Este é um jogo estilo **Space Invaders** desenvolvido para o microcontrolador **STM32F411CE (Black Pill)** utilizando o sistema operacional de tempo real **FreeRTOS (CMSIS-RTOS V2)**. O projeto foca em boas práticas de sistemas embarcados, demonstrando sincronização atômica de tarefas, temporização precisa e otimização de periféricos via hardware.

---

## 🎨 Arquitetura de Sincronização de Tarefas (RTOS)

O ecossistema do jogo opera sem travar a CPU. O fluxo de dados foi dividido em tarefas especializadas que conversam através de mecanismos de sincronização (Semáforos e Mutex):

```mermaid
graph TD
    subgraph Entradas
        A[vTask_Input] -->|Modifica Estado| B(Struct Game)
    end
    
    subgraph Processamento Lógico e Visual
        B -->|memcpy protegido por Mutex| C(game_render_copy)
        D[vTask_Game] -->|Calcula Física 60Hz| B
        D -->|Sinaliza Semáforo| E[vTask_Render]
        E -->|Lê dados sob Mutex| C
        E -->|Desenha via SPI| F[Display LCD ST7735]
    end

    subgraph Sonoplastia
        B -->|Lê Estado| G[vTask_SoundMenu]
        G -->|Gera PWM| H[Buzzer]
    end

    style B fill:#f9f,stroke:#333,stroke-width:2px
    style C fill:#bbf,stroke:#333,stroke-width:2px
    style D fill:#ff9,stroke:#333,stroke-width:2px
    style E fill:#9f9,stroke:#333,stroke-width:2px
