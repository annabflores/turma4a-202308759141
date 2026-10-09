# Conjunto de Instruções e Arquitetura de Memória (ISA) 
**Trabalho AP1 — Arquitetura de Computadores** 
**Projeto:** Sistema Programável de Três Bombas (Arduino Mega 2560) 

--- 

## 1. Largura da Palavra (\\(W\\)) e Capacidade de Memória

* **Largura Fixa da Palavra (\\(W\\)):** **16 bits** (\\(2\text{ bytes}\\)).
* **Capacidade Lógica da Memória (`MEM_PROG`):** 128 posições de largura fixa de 16 bits, totalizando \\(128 \times 16 = 2048\text{ bits}\\) (\\(256\text{ bytes}\\)).
* **Justificativa de \\(W = 16\text{ bits}\\):** A escolha de 16 bits permite acomodar com folga o opcode de 4 bits (suportando até 16 instruções distintas) e todos os operandos exigidos pela aplicação (como endereços de 7 bits para atingir até \\(0\text{x}7\text{F}\\) e percentuais inteiros de 0 a 100%), além de possibilitar um alinhamento claro em grupos de 4 nibbles hexadecimais no monitor serial.

---

## 2. Estrutura e Formato dos Campos na Palavra (16 bits) 

Toda palavra binária armazenada na `MEM_PROG` possui 16 bits organizados conforme os formatos abaixo: 

### Formato Tipo R (Registrador / Ação de Bomba) Utilizado para instruções de acionamento, leitura e sinalização (`READ`, `ON`, `OFF`, `ALARM`, `LED`, `INFO`, `SILENCE`, `LEDOFF`). 
* **Opcode `[15:12]` (4 bits):** Código de operação da instrução.
* **Reservado `[11:2]` (10 bits):** Preenchido obrigatoriamente com zeros (`0000000000`).
* * **Bomba `[1:0]` (2 bits):** Identificador do reservatório/bomba (`01` = Bomba 1, `10` = Bomba 2, `11` = Bomba 3).
 
### Formato Tipo C (Comparação) 
Utilizado exclusivamente pela instrução `CMP x, n`. 
* **Opcode `[15:12]` (4 bits):** Opcode da comparação (`1000`).
* **Bomba `[11:10]` (2 bits):** Identificador da bomba (`01`, `10` ou `11`).
* **Reservado `[9:7]` (3 bits):** Preenchido obrigatoriamente com zeros (`000`).
* **Percentual `n` `[6:0]` (7 bits):** Valor inteiro de percentual para comparação (faixa decimal de 0 a 100, equivalente a `0000000` até `1100100`).
