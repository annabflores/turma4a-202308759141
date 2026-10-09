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

### Formato Tipo J (Desvio / Controle) 
Utilizado por instruções de salto (`JMP`, `JL`, `JE`, `JG`). 
* **Opcode `[15:12]` (4 bits):** Código da instrução de desvio.
* **Reservado `[11:7]` (5 bits):** Preenchido obrigatoriamente com zeros (`00000`).
* **Endereço `a` `[6:0]` (7 bits):** Endereço de destino na memória (faixa de `0x00` a `0x7F`, ou seja, 0 a 127 em decimal).

### Formato Tipo T (Temporização) 
Utilizado pela instrução de espera (`WAIT t`). 
* **Opcode `[15:12]` (4 bits):** Código da instrução (`1101`).
* **Reservado `[11:7]` (5 bits):** Preenchido obrigatoriamente com zeros (`00000`).
* **Tempo `t` `[6:0]` (7 bits):** Tempo de espera em múltiplos de 100 ms (faixa de 0 a 127). 

### Formato Tipo S (Sem Operandos) 
Utilizado pela instrução de parada (`HALT`). 
* **Opcode `[15:12]` (4 bits):** Código da instrução (`1110`).
* **Reservado `[11:0]` (12 bits):** Preenchido obrigatoriamente com zeros (`000000000000`). 

--- 

## 3. Tabela do Dicionário Binário da ISA 

| Mnemônico | Opcode (4 bits) | Formato dos 16 bits `[15 ... 0]` 
| Descrição Funcional | 
| :--- | :---: | :--- | :--- | 
| **`READ x`** | `0000` | `0000 0000 0000 00xx` | Lê o nível analógico do reservatório \\(x\\), converte para percentual inteiro, armazena em \`NIVEL[x]\` e \`ACC\` e marca leitura válida. 
| 
| **`ON x`** | `0001` | `0001 0000 0000 00xx` | Liga o LED representante da bomba \\(x\\). | 
| **`OFF x`** | `0010` | `0010 0000 0000 00xx` | Desliga o LED representante da bomba \\(x\\). | 
| **`ALARM x`** | `0011` | `0011 0000 0000 00xx` | Ativa estado de manutenção corretiva da bomba \\(x\\) e o buzzer. | 
| **`LED x`** | `0100` | `0100 0000 0000 00xx` | Liga o LED de manutenção preventiva da bomba \\(x\\). | 
| **`INFO x`** | `0101` | `0101 0000 0000 00xx` | Atualiza leitura da bomba \\(x\\), guarda em `ACC`/`NIVEL[x]` e exibe a faixa numérica no display. | 
| **`SILENCE x`** | `0110` | `0110 0000 0000 00xx` | Desativa alarme corretivo da bomba \\(x\\) (buzzer desliga se não houver outros alarmes ativos). | 
| **`LEDOFF x`** | `0111` | `0111 0000 0000 00xx` | Desliga o LED de manutenção preventiva da bomba \\(x\\). | 
| **`CMP x, n`** | `1000` | `1000 xx00 0nnnnnnn` | Compara `NIVEL[x]` com \\(n\\), guarda a diferença em `ACC` e atualiza os flags `FLAG_L`, `FLAG_Z` e `FLAG_G`. | 
| **`JMP a`** | `1001` | `1001 0000 0aaaaaaa` | Desvia incondicionalmente para o endereço \\(a\\) (`0x00` a `0x7F`). | 
| **`JL a`** | `1010` | `1010 0000 0aaaaaaa` | Desvia para o endereço \\(a\\) se `FLAG_L` for verdadeiro. | 
| **`JE a`** | `1011` | `1011 0000 0aaaaaaa` | Desvia para o endereço \\(a\\) se `FLAG_Z` for verdadeiro. | 
| **`JG a`** | `1100` | `1100 0000 0aaaaaaa` | Desvia para o endereço \\(a\\) se `FLAG_G` for verdadeiro. | 
| **`WAIT t`** | `1101` | `1101 0000 0ttttttt` | Aguarda \\(t \times 100\text{ ms}\\) sem bloquear comandos do monitor serial. | 
| **`HALT`** | `1110` | `1110 0000 0000 0000` | Encerra a execução do programa e mantém o estado das saídas. | 
| *(Inválido)* | `1111` | `1111 xxxx xxxx xxxx` | Opcode reservado para detecção de instrução/palavra corrompida. | 

--- 

## 4. Regras de Preenchimento e Validação 
1. **Campos Não Utilizados:** Todos os bits não mapeados para operandos em uma instrução específica são obrigatoriamente preenchidos com zeros (`0`) no processo de montagem.
2. **Detecção de Combinações Inválidas:** Durante o ciclo de busca e decodificação, se a Unidade de Controle (UC) encontrar o opcode `1111` ou tentar acessar uma posição não carregada da memória, a execução é interrompida imediatamente, desativando bombas e alarmes e emitindo um diagnóstico de erro no monitor serial.
3. **Representação dos Endereços:** Os endereços das posições de instrução usam rigorosamente a notação hexadecimal de dois dígitos com prefixo `0x` (de `0x00` a `0x7F`).

--- 

## 5. Exemplos de Codificação Binária Completa 

### Exemplo 1: `READ 1` (Instrução gravada no endereço `0x00`) 
* **Mnemônico:** `READ 1`
* **Opcode (4 bits):** `0000`
* **Reservado (10 bits):** `0000000000`
* **Bomba 1 (2 bits):** `01`
* **Palavra armazenada (16 bits):** `0000000000000001`
* **Representação Hexadecimal da Palavra:** `0x0001`

### Exemplo 2: `CMP 2, 80` (Instrução gravada no endereço `0x01`) 
* **Mnemônico:** `CMP 2, 80`
* **Opcode (4 bits):** `1000`
* **Bomba 2 (2 bits):** `10`
* **Reservado (3 bits):** `000`
* **Percentual 80 em binário (7 bits):** \\(80_{10} = 1010000_2\\)
* **Palavra armazenada (16 bits):** `1000100001010000`
* **Representação Hexadecimal da Palavra:** `0x8850` 

### Exemplo 3: `JMP 0x05` (Instrução gravada no endereço `0x02`) 
* **Mnemônico:** `JMP 0x05`
* **Opcode (4 bits):** `1001`
* **Reservado (5 bits):** `00000`
* **Endereço 0x05 em binário (7 bits):** \\(5\_{10} = 0000101_2\\)
* **Palavra armazenada (16 bits):** `1001000000000101`
* **Representação Hexadecimal da Palavra:** `0x9005`
