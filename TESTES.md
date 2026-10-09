# Relatório de Testes e Programas em Assembly
**Trabalho AP1 — Arquitetura de Computadores**  
**Projeto:** Sistema Programável de Três Bombas (Arduino Mega 2560)  

---

## 1. Visão Geral
Este documento apresenta os cenários de teste, os programas em Assembly utilizados e a validação do processador simulado de 16 bits implementado no Arduino Mega 2560. Todos os endereços de instrução utilizam a notação hexadecimal de dois dígitos com prefixo `0x` (de `0x00` a `0x7F`).

---

## 2. Relação de Testes Executados

### 2.1 Teste de Carga, Montagem e Comentários (Requisito 4.1)
* **Objetivo:** Comprovar a carga do programa armazenado, a não ativação imediata das saídas durante a carga, a remoção de comentários e linhas vazias, e a consulta de memória.
* **Programa em Assembly enviado (`LOAD`):**
  ```assembly
  ; Programa de Teste de Carga e Comentarios
  READ 1 ; Le nivel do reservatorio 1 no endereco 0x00
  
  ON 1   ; Liga a bomba 1 no endereco 0x01
  OFF 1  ; Desliga a bomba 1 no endereco 0x02
  HALT   ; Encerra a execucao no endereco 0x03
  ```

* **Resultados das Palavras Binárias Geradas:**
  * Endereço `0x00`: `0000 0000 0000 0001` (`READ 1`)
  * Endereço `0x01`: `0001 0000 0000 0001` (`ON 1`)
  * Endereço `0x02`: `0010 0000 0000 0001` (`OFF 1`)
  * Endereço `0x03`: `1110 0000 0000 0000` (`HALT`)

* **Validação:**
  * **Resultado Esperado:** Nenhuma saída (LED/Buzzer) ativada durante a digitação no modo `LOAD`. O comando `END` confirma 4 posições ocupadas (`0x00` a `0x03`).
  * **Resultado Obtido:** Aprovado. Linhas de comentários e espaços em branco foram ignorados sem avançar o ponteiro de carga. O comando `MEM 0x00 0x03` exibiu exatamente as 4 palavras binárias de 16 bits.

---

### 2.2 Ciclo de Instrução e Execução Passo a Passo (Requisito 4.2)
* **Objetivo:** Demonstrar o ciclo de busca, decodificação e execução via `RUN` e `STEP` (ou `*`).
* **Procedimento:** Executou-se o programa anterior utilizando os comandos `RUN` e `STEP`.
* **Saída do Monitor Serial Observada:**
 
  [SISTEMA] Sessao PASSO A PASSO iniciada no endereco 0x00.
  > STEP
  EXEC: 0x00 | IR: 0000 0000 0000 0001 | DEC: READ 1 | ACC: 45 | FLAGS [L:0 Z:0 G:0 VAL:0] | Prox PC: 0x01
  > STEP
  EXEC: 0x01 | IR: 0001 0000 0000 0001 | DEC: ON 1   | ACC: 45 | FLAGS [L:0 Z:0 G:0 VAL:0] | Prox PC: 0x02
  > STEP
  EXEC: 0x02 | IR: 0010 0000 0000 0001 | DEC: OFF 1  | ACC: 45 | FLAGS [L:0 Z:0 G:0 VAL:0] | Prox PC: 0x03
  > STEP
  EXEC: 0x03 | IR: 1110 0000 0000 0000 | DEC: HALT   | ACC: 45 | FLAGS [L:0 Z:0 G:0 VAL:0]
  [SISTEMA] Programa encerrado normalmente por instrucao HALT.

* **Resultado Esperado vs. Obtido:** Aprovado. A busca leu a memória no endereço indicado pelo `PC`, isolou os bits do `IR` e incrementou o `PC` corretamente.

---

### 2.3 Leitura Analógica Independente (Requisito 4.3)
* **Objetivo:** Verificar a conversão A/D para percentual inteiro (0% a 100%) nas três entradas analógicas (`A0`, `A1`, `A2`).
* **Programa em Assembly enviado (`LOAD`):**
 
  READ 1 ; Nivel da Bomba 1 -> A0
  READ 2 ; Nivel da Bomba 2 -> A1
  READ 3 ; Nivel da Bomba 3 -> A2
  HALT

* **Cenários Testados:**
  1. Potenciômetros girados aos extremos (0V e 5V): Lidos como `0%` e `100%` nos registradores `NIVEL[x]` e `ACC`.
  2. Níveis intermediários aleatórios: Leitura analógica convertida e armazenada com precisão.
* **Resultado Esperado vs. Obtido:** Aprovado. A fórmula de conversão garantiu mapeamento exato na faixa de 0 a 100%.

---

### 2.4 Comparação, Flags da ULA e Desvios Condicionais (Requisito 4.4)
* **Objetivo:** Validar a instrução `CMP x, n`, a atualização do `ACC` com a diferença (`NIVEL[x] - n`) em complemento de 2, a atualização das flags e o salto condicional (`JL`, `JE`, `JG`).
* **Programa em Assembly enviado (`LOAD`):**
 
  READ 1      ; Endereço 0x00: Le nivel atual da bomba 1 (Exemplo lido: 20%)
  CMP 1, 30   ; Endereço 0x01: Compara 20% com 30% -> ACC = 20 - 30 = -10 (FLAG_L = 1)
  JL 0x04     ; Endereço 0x02: Desvia para 0x04 se Menor (FLAG_L)
  OFF 1       ; Endereço 0x03: Nao deve ser executado se o desvio ocorrer
  ON 1        ; Endereço 0x04: Alvo do desvio condicional
  HALT        ; Endereço 0x05

* **Validação de Complemento de 2 e Flags:**
  * **Cenário Nível < Limite (20% < 30%):** `ACC = -10` (representado internamente em complemento de 2), `FLAG_L = 1`, `FLAG_Z = 0`, `FLAG_G = 0`. O desvio em `0x02` para `0x04` foi executado com sucesso, saltando a instrução `OFF 1` em `0x03`.
* **Resultado Esperado vs. Obtido:** Aprovado. Exatamente uma flag permaneceu verdadeira e os saltos condicionais operaram conforme a regra da ULA.

---

### 2.5 Gerenciamento de Bombas, LEDs Preventivos e Alarmes Corretivos (Requisito 4.5)
* **Objetivo:** Validar independência dos acionamentos das bombas, LEDs de manutenção preventiva e lógica de silenciamento do buzzer.
* **Programa em Assembly enviado (`LOAD`):**

  ALARM 1   ; Endereço 0x00: Ativa alarme corretivo da bomba 1 e liga Buzzer
  ALARM 2   ; Endereço 0x01: Ativa alarme corretivo da bomba 2 (Buzzer continua ligado)
  SILENCE 1 ; Endereço 0x02: Desativa alarme 1 (Buzzer PERMANECE LIGADO devido ao Alarme 2)
  SILENCE 2 ; Endereço 0x03: Desativa alarme 2 (Buzzer DESLIGA pois nao ha mais alarmes ativos)
  HALT      ; Endereço 0x04

* **Resultado Esperado vs. Obtido:** Aprovado. O buzzer permaneceu acionado em `SILENCE 1` e só foi desligado em `SILENCE 2`, comprovando a lógica de controle compartilhado de alarmes.

---

### 2.6 Mapeamento do Display de 7 Segmentos (Requisito 4.6)
* **Objetivo:** Testar os limites de faixa da instrução `INFO x` e a exibição do dígito no display de 7 segmentos.
* **Tabela de Teste dos Limites de Mapeamento:**

| Percentual Lido (%) | Dígito Esperado | Dígito Obtido | Estado do Display |
| :---: | :---: | :---: | :---: |
| 0% e 10% | `1` | `1` | Aceso corretamente |
| 11% e 20% | `2` | `2` | Aceso corretamente |
| 21% e 30% | `3` | `3` | Aceso corretamente |
| 31% e 40% | `4` | `4` | Aceso corretamente |
| 41% e 50% | `5` | `5` | Aceso corretamente |
| 51% e 60% | `6` | `6` | Aceso corretamente |
| 61% e 70% | `7` | `7` | Aceso corretamente |
| 71% e 90% | `8` | `8` | Aceso corretamente |
| 91% e 100% | `9` | `9` | Aceso corretamente |

* **Resultado Esperado vs. Obtido:** Aprovado. Todos os limites de transição foram validados.

---

### 2.7 Consulta de Memória e Capacidade Limite (Requisito 4.7)
* **Objetivo:** Testar os comandos `MEM` e `MEM 0x00 0x0F` e a rejeição de carga além das 128 posições (`0x7F`).
* **Procedimento:**
  1. Executou-se `MEM 0x00 0x03` para consultar um intervalo específico.
  2. Tentou-se carregar 129 instruções seguidas no modo `LOAD`.
* **Resultado Esperado vs. Obtido:** Aprovado. O montador rejeitou a 129ª instrução emitindo a mensagem `[ERRO MONTADOR] Memoria de programa cheia`. A consulta com `MEM` não alterou registradores, `PC` ou estados de saída.

---

### 2.8 Tratamento de Erros e Exceções (Requisito 4.8)
* **Objetivo:** Garantir que o sistema trate e rejeite falhas sintáticas ou de execução sem travar o microcontrolador.

| Cenário de Teste | Entrada Enviada | Comportamento Esperado | Resultado Obtido |
| :--- | :--- | :--- | :--- |
| **Mnemônico Desconhecido** | `TESTE 1` | Rejeição no `LOAD` sem avançar o ponteiro | Aprovado |
| **Bomba Inexistente** | `READ 4` | Rejeição no `LOAD` (Somente bombas 1, 2 e 3) | Aprovado |
| **Endereço sem `0x`** | `JMP 05` | Rejeição no `LOAD` (Exige notação `0x05`) | Aprovado |
| **CMP sem Leitura Prévia** | `CMP 1, 50` (Sem `READ`) | Interrupção da execução no `RUN`/`AUTO` com erro | Aprovado |
| **Desvio sem Comparação** | `JL 0x05` (Sem `CMP`) | Interrupção da execução no `RUN`/`AUTO` com erro | Aprovado |
| **Acesso a Endereço não carregado** | `JMP 0x10` (Endereço livre) | Detecção de erro pelo montador no `END` | Aprovado |

---

### 2.9 Programa de Controle Automático em Laço (Requisito 4.9)
* **Objetivo:** Implementar um programa armazenado completo que lê continuamente os níveis dos 3 reservatórios, liga cada bomba se o nível for **menor que 30%**, desliga se for **maior que 80%**, e **mantém o estado atual** se estiver na faixa entre 30% e 80% (inclusive).

* **Código em Assembly Armazenado (`LOAD`):**

  ; PROGRAMA DE CONTROLE AUTOMATICO DAS TRES BOMBAS
  ; Regra: Nivel < 30% -> Liga | Nivel > 80% -> Desliga | 30% a 80% -> Mantem
  ; 

  ; --- CONTROLE DA BOMBA 1 ---
  READ 1       ; Endereco 0x00: Le nivel da Bomba 1
  CMP 1, 29    ; Endereco 0x01: Compara com 29% (Se <= 29%, ou seja < 30%)
  JL 0x06      ; Endereco 0x02: Se Menor/Igual a 29%, vai para LIGA_B1 (0x06)
  CMP 1, 80    ; Endereco 0x03: Compara com 80%
  JG 0x08      ; Endereco 0x04: Se Maior que 80%, vai para DESLIGA_B1 (0x08)
  JMP 0x09     ; Endereco 0x05: Entre 30% e 80% -> Mantem estado e vai para B2

  ; [0x06] LIGA_B1
  ON 1         ; Endereco 0x06: Liga Bomba 1
  JMP 0x09     ; Endereco 0x07: Vai para controle da Bomba 2

  ; [0x08] DESLIGA_B1
  OFF 1        ; Endereco 0x08: Desliga Bomba 1

  ; --- CONTROLE DA BOMBA 2 ---
  READ 2       ; Endereco 0x09: Le nivel da Bomba 2
  CMP 2, 29    ; Endereco 0x0A: Compara com 29%
  JL 0x0F      ; Endereco 0x0B: Se < 30%, vai para LIGA_B2 (0x0F)
  CMP 2, 80    ; Endereco 0x0C: Compara com 80%
  JG 0x11      ; Endereco 0x0D: Se > 80%, vai para DESLIGA_B2 (0x11)
  JMP 0x12     ; Endereco 0x0E: Mantem estado e vai para B3

  ; [0x0F] LIGA_B2
  ON 2         ; Endereco 0x0F: Liga Bomba 2
  JMP 0x12     ; Endereco 0x10: Vai para controle da Bomba 3

  ; [0x11] DESLIGA_B1
  OFF 2        ; Endereco 0x11: Desliga Bomba 2

  ; --- CONTROLE DA BOMBA 3 ---
  READ 3       ; Endereco 0x12: Le nivel da Bomba 3
  CMP 3, 29    ; Endereco 0x13: Compara com 29%
  JL 0x18      ; Endereco 0x14: Se < 30%, vai para LIGA_B3 (0x18)
  CMP 3, 80    ; Endereco 0x15: Compara com 80%
  JG 0x1A      ; Endereco 0x16: Se > 80%, vai para DESLIGA_B3 (0x1A)
  JMP 0x1B     ; Endereco 0x17: Mantem estado e encerra ciclo

  ; [0x18] LIGA_B3
  ON 3         ; Endereco 0x18: Liga Bomba 3
  JMP 0x1B     ; Endereco 0x19: Encerra ciclo

  ; [0x1A] DESLIGA_B3
  OFF 3        ; Endereco 0x1A: Desliga Bomba 3

  ; --- TEMPORIZACAO E LACO CONTINUO ---
  WAIT 10      ; Endereco 0x1B: Aguarda 1.0 segundo (10 x 100 ms)
  JMP 0x00     ; Endereco 0x1C: Reinicia o laço de controle em 0x00
 

* **Validação do Controle Automático (`AUTO`):**
  1. **Ajuste dos Potenciômetros:** Variou-se o nível dos reservatórios simulados durante a execução no modo `AUTO`.
  2. **Comportamento Observado:** Quando o reservatório caiu para 25% (<30%), a bomba respectiva ligou imediatamente. Ao subir e passar por 50% (faixa de 30% a 80%), a bomba **permaneceu ligada**. Ao ultrapassar 80% (ex: 85%), a bomba foi desligada.
  3. **Interrupção via `STOP`:** O envio do comando `STOP` no monitor serial interrompeu a execução e desativou todas as bombas no meio do laço.

---

### 2.10 Instrução HALT e Avaliação Conceitual (Requisito 4.10)
* **Objetivo:** Comprovar a paralisação do processador após a instrução `HALT` em programas finitos.
* **Resultado Esperado vs. Obtido:** Aprovado. Após atingir a instrução `HALT`, novos envios do comando `STEP` ou `*` foram ignorados pelo processador, mantendo o estado dos registradores e saídas inalterado.



