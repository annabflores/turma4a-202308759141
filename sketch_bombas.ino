
* TRABALHO AP1 — ARQUITETURA DE COMPUTADORES 
* SISTEMA PROGRAMÁVEL DE TRÊS BOMBAS (ARDUINO MEGA 2560)

* IDENTIFICAÇÃO DO GRUPO: 
* Turma: turma4a 
* Representante: - Matrícula: - Participação: TA 
* Integrante 2: - Matrícula: - Participação: TA 
* Integrante 3: - Matrícula: - Participação: TA 
* Integrante 4: - Matrícula: - Participação: TA 

#include <Arduino.h>

// 1. PINAGEM OFICIAL DE REFERÊNCIA (SEÇÃO 3.3 DO DOCUMENTO DE REQUISITOS) //

const uint8_t PIN_POT_B1 = A0; // Potenciômetro Reservatório 1
const uint8_t PIN_POT_B2 = A1; // Potenciômetro Reservatório 2
const uint8_t PIN_POT_B3 = A2; // Potenciômetro Reservatório 3

// Segmentos do Display de 7 Segmentos (a, b, c, d, e, f, g)
const uint8_t PIN_DISP[7] = {22, 23, 24, 25, 26, 27, 28};

// LEDs das Bombas (Acionamento Principal)
const uint8_t PIN_BOMBA[4] = {0, 30, 31, 32}; // Índices 1, 2, 3

// LEDs de Manutenção Preventiva
const uint8_t PIN_PREV[4] = {0, 33, 34, 35}; // Índices 1, 2, 3

// Controle do Buzzer (Alarmes Corretivos)
const uint8_t PIN_BUZZER = 45;

// 2. ELEMENTOS ARQUITETURAIS SIMULADOS E REGISTRADORES (SEÇÃO 2.1) //

#define MEM_SIZE 128 // Memória de Programa: 128 palavras de largura fixa W = 16 bits
uint16_t MEM_PROG[MEM_SIZE];
bool CARREGADO[MEM_SIZE]; // Controle de ocupação separado do conteúdo zerado

// Registradores do Processador Simulado
uint8_t PC = 0x00;        // Contador de Programa (0x00 a 0x7F)
uint16_t IR = 0x0000;     // Registrador de Instrução
int16_t ACC = 0;          // Acumulador (Suporta diferenças de -100 a +100 em Complemento de 2)

// Registradores de Nível e Validade das Leituras
uint8_t NIVEL[4] = {0, 0, 0, 0};
bool NIVEL_VALIDO[4] = {false, false, false, false};

// Flags de Comparação da ULA e Validade
bool FLAG_L = false;       // Menor que (Less)
bool FLAG_Z = false;       // Igual a (Zero)
bool FLAG_G = false;       // Maior que (Greater)
bool COMP_VALIDA = false;  // Indica se há uma comparação ativa válida

// Estados dos Alarmes Corretivos (para controle do Buzzer)
bool ALARM_CORRETIVO[4] = {false, false, false, false};

// Controle de Estado do Sistema e Ponteiro de Carga
enum ModoOperacao { MODO_IDLE, MODO_LOAD, MODO_RUN_STEP, MODO_AUTO, MODO_HALT, MODO_ERRO };
ModoOperacao modo_atual = MODO_IDLE;
uint8_t ponteiro_carga = 0x00;
bool pendencias_desvio = false;

// Controle de Temporização (WAIT t)
unsigned long tempo_espera_fim = 0;
bool em_espera = false;

// 3. OPCODES DA ISA ARBITRADA (LARGURA DE PALAVRA W = 16 BITS) //

const uint8_t OP_READ = 0b0000;
const uint8_t OP_ON = 0b0001;
const uint8_t OP_OFF = 0b0010;
const uint8_t OP_ALARM = 0b0011;
const uint8_t OP_LED = 0b0100;
const uint8_t OP_INFO = 0b0101;
const uint8_t OP_SILENCE = 0b0110;
const uint8_t OP_LEDOFF = 0b0111;
const uint8_t OP_CMP = 0b1000;
const uint8_t OP_JMP = 0b1001;
const uint8_t OP_JL = 0b1010;
const uint8_t OP_JE = 0b1011;
const uint8_t OP_JG = 0b1100;
const uint8_t OP_WAIT = 0b1101;
const uint8_t OP_HALT = 0b1110;
const uint8_t OP_INVALID = 0b1111;

// PROTÓTIPOS DAS FUNÇÕES
void desligarTodasSaidas();
void apagarDisplay();
void atualizarBuzzer();
void atualizarDisplay7Seg(uint8_t digito);
uint8_t lerNivelPercentual(uint8_t bomba);
void imprimirHexDoisDigitos(uint8_t val);
void imprimirBinario16Bits(uint16_t val);
void tratarComandoAmbiente(String cmd);
void processarLinhaAssembly(String linha);
bool validarEDecodificarAssembly(String linha, uint16_t &palavra_binaria);
void executarCicloInstrucao();
void exibirStatus();
void consultarMemoria(int inicio, int fim);

// 4. SETUP E LOOP PRINCIPAL //

void setup() {
  Serial.begin(9600);
  // Configuração dos Pinos
  for (int i = 0; i < 7; i++)
    pinMode(PIN_DISP[i], OUTPUT);
  for (int b = 1; b <= 3; b++) {
    pinMode(PIN_BOMBA[b], OUTPUT);
    pinMode(PIN_PREV[b], OUTPUT);
  }
  pinMode(PIN_BUZZER, OUTPUT);
  desligarTodasSaidas();
  apagarDisplay();

  Serial.println(F("\n========================================================"));
  Serial.println(F(" TRABALHO AP1 - SISTEMA DE TRES BOMBAS (ARDUINO MEGA) "));
  Serial.println(F(" Processador Simulado de 16 bits | Memoria de 128 palavras"));
  Serial.println(F(" Digite 'LOAD' para iniciar a carga do programa."));
  Serial.println(F("========================================================\n"));
}

void loop() {
  // Leitura e Processamento de Comandos via Monitor Serial
  if (Serial.available() > 0) {
    String entrada = Serial.readStringUntil('\n');
    entrada.trim();
    if (entrada.length() > 0) {
      if (modo_atual == MODO_LOAD) {
        // No modo LOAD, se o comando for END, encerra a carga; caso contrário, monta a linha
        if (entrada.equalsIgnoreCase("END")) {
          tratarComandoAmbiente("END");
        } else {
          processarLinhaAssembly(entrada);
        }
      } else {
        tratarComandoAmbiente(entrada);
      }
    }
  }

  // Tratamento de Temporização Não-Bloqueante (WAIT t)
  if (em_espera) {
    if (millis() >= tempo_espera_fim) {
      em_espera = false;
      PC++; // Avança PC após término da espera
      if (modo_atual == MODO_AUTO) {
        // Continua a execução no modo automático
      }
    }
    return; // Enquanto espera, ignora passos de execução do AUTO
  }

  // Execução Contínua no Modo AUTO
  if (modo_atual == MODO_AUTO && !em_espera) {
    executarCicloInstrucao();
    delay(50); // Pequeno intervalo entre ciclos no AUTO
  }
}

void desligarTodasSaidas() {
  for (int b = 1; b <= 3; b++) {
    digitalWrite(PIN_BOMBA[b], LOW);
    digitalWrite(PIN_PREV[b], LOW);
    ALARM_CORRETIVO[b] = false;
  }
  atualizarBuzzer();
  apagarDisplay();
}

void apagarDisplay() {
  for (int i = 0; i < 7; i++)
    digitalWrite(PIN_DISP[i], LOW);
}

void atualizarBuzzer() {
  bool ativo = (ALARM_CORRETIVO[3] || ALARM_CORRETIVO[4] || ALARM_CORRETIVO[5]);
  digitalWrite(PIN_BUZZER, ativo ? HIGH : LOW);
}

void atualizarDisplay7Seg(uint8_t digito) {
  // Mapeamento dos segmentos a-g para catodo comum (HIGH = Aceso)
  const uint8_t mapa_segmentos[6][1] = {
    {0,0,0,0,0,0,0}, // 0: Apagado
    {0,1,1,0,0,0,0}, // 1
    {1,1,0,1,1,0,1}, // 2
    {1,1,1,1,0,0,1}, // 3
    {0,1,1,0,0,1,1}, // 4
    {1,0,1,1,0,1,1}, // 5
    {1,0,1,1,1,1,1}, // 6
    {1,1,1,0,0,0,0}, // 7
    {1,1,1,1,1,1,1}, // 8
    {1,1,1,1,0,1,1}  // 9
  };
  
  if (digito >= 1 && digito <= 9) {
    for (int i = 0; i < 7; i++) {
      digitalWrite(PIN_DISP[i], mapa_segmentos[digito][i] ? HIGH : LOW);
    }
  } else {
    apagarDisplay();
  }
}

uint8_t lerNivelPercentual(uint8_t bomba) {
  int pino = (bomba == 1) ? PIN_POT_B1 : ((bomba == 2) ? PIN_POT_B2 : PIN_POT_B3);
  int leitura = analogRead(pino);
  // Conversão da leitura analógica (0-1023) para percentual inteiro (0-100)
  uint32_t perc = ((uint32_t)leitura * 100UL + 511UL) / 1023UL;
  if (perc > 100) perc = 100;
  return (uint8_t)perc;
}

// Mapeamento de percentual para dígito do display conforme Tabela 2.5 
uint8_t percentualParaDigito(uint8_t perc) {
  if (perc <= 10) return 1;
  if (perc <= 20) return 2;
  if (perc <= 30) return 3;
  if (perc <= 40) return 4;
  if (perc <= 50) return 5;
  if (perc <= 60) return 6;
  if (perc <= 70) return 7;
  if (perc <= 90) return 8;
  return 9; // 91 a 100%
}

// 6. FORMATAÇÃO DE SAÍDA NO MONITOR SERIAL //

void imprimirHexDoisDigitos(uint8_t val) {
  Serial.print(F("0x"));
  if (val < 0x10) Serial.print(F("0"));
  Serial.print(val, HEX);
}

void imprimirBinario16Bits(uint16_t val) {
  for (int i = 15; i >= 0; i--) {
    Serial.print((val >> i) & 0x01);
    if (i == 12 || i == 8 || i == 4) Serial.print(F(" "));
  }
}

```

// 7. COMANDOS DO AMBIENTE (LOAD, END, RUN, STEP, AUTO, STOP, MEM, STATUS) //

void tratarComandoAmbiente(String cmd) {
  String cmdUpper = cmd;
  cmdUpper.toUpperCase();
  if (cmdUpper == "LOAD") {
    desligarTodasSaidas();
    modo_atual = MODO_LOAD;
    ponteiro_carga = 0x00;
    pendencias_desvio = false;
    em_espera = false;
    PC = 0x00;
    for (int i = 0; i < MEM_SIZE; i++) {
      MEM_PROG[i] = 0x0000;
      CARREGADO[i] = false;
    }
    Serial.println(F("[SISTEMA] Modo LOAD iniciado. Memoria zerada e ponteiro em 0x00."));
    Serial.println(F("[SISTEMA] Digite o programa em Assembly linha por linha. Digite 'END' para finalizar."));
  } else if (cmdUpper == "END") {
    if (modo_atual != MODO_LOAD) {
      Serial.println(F("[ERRO] Comando END so e valido durante o modo LOAD."));
      return;
    }
    modo_atual = MODO_IDLE;
    // Validação dos destinos de desvios
    pendencias_desvio = false;
    for (int i = 0; i < MEM_SIZE; i++) {
      if (CARREGADO[i]) {
        uint8_t op = (MEM_PROG[i] >> 12) & 0x0F;
        if (op == OP_JMP || op == OP_JL || op == OP_JE || op == OP_JG) {
          uint8_t dest = MEM_PROG[i] & 0x7F;
          if (!CARREGADO[dest]) {
            Serial.print(F("[ALERTA MONTADOR] Pendencia no endereco "));
            imprimirHexDoisDigitos(i);
            Serial.print(F(": Desvio para "));
            imprimirHexDoisDigitos(dest);
            Serial.println(F(" nao carregado na memoria!"));
            pendencias_desvio = true;
          }
        }
      }
    }
    Serial.print(F("[SISTEMA] Carga finalizada. Total de posicoes ocupadas: "));
    Serial.println(ponteiro_carga);
    if (pendencias_desvio) {
      Serial.println(F("[ERRO] Existem pendencias de desvio! O programa nao pode ser executado ate nova carga."));
    } else {
      Serial.println(F("[SISTEMA] Programa validado com sucesso! Pronto para 'RUN' ou 'AUTO'."));
    }
  } else if (cmdUpper == "RUN") {
    if (pendencias_desvio) {
      Serial.println(F("[ERRO] Execucao bloqueada devido a pendencias de desvio nao carregados."));
      return;
    }
    modo_atual = MODO_RUN_STEP;
    PC = 0x00;
    em_espera = false;
    desligarTodasSaidas();
    // Reinicializa registradores e validade
    ACC = 0;
    COMP_VALIDA = false;
    FLAG_L = FLAG_Z = FLAG_G = false;
    for (int b = 1; b <= 3; b++) NIVEL_VALIDO[b] = false;
    Serial.println(F("[SISTEMA] Sessao PASSO A PASSO iniciada no endereco 0x00. Envie 'STEP' ou '*' para executar."));
  } else if (cmdUpper == "STEP" || cmdUpper == "*") {
    if (modo_atual != MODO_RUN_STEP) {
      Serial.println(F("[ERRO] Comandos STEP/* so funcionam no modo RUN. Use 'RUN' para iniciar."));
      return;
    }
    if (em_espera) {
      Serial.println(F("[REJEITADO] Instrucao em WAIT ativo. Aguarde o tempo antes de novo STEP."));
      return;
    }
    executarCicloInstrucao();
  } else if (cmdUpper == "AUTO") {
    if (pendencias_desvio) {
      Serial.println(F("[ERRO] Execucao bloqueada devido a pendencias de desvio nao carregados."));
      return;
    }
    modo_atual = MODO_AUTO;
    PC = 0x00;
    em_espera = false;
    desligarTodasSaidas();
    ACC = 0;
    COMP_VALIDA = false;
    FLAG_L = FLAG_Z = FLAG_G = false;
    for (int b = 1; b <= 3; b++) NIVEL_VALIDO[b] = false;
    Serial.println(F("[SISTEMA] Execucao CONTINUA (AUTO) iniciada em 0x00. Envie 'STOP' para interromper."));
  } else if (cmdUpper == "STOP") {
    desligarTodasSaidas();
    modo_atual = MODO_HALT;
    em_espera = false;
    Serial.println(F("[SISTEMA] Execucao interrompida. Bombas e alarmes desligados."));
  } else if (cmdUpper.startsWith("MEM")) {
    int i_start = 0, i_end = MEM_SIZE - 1;
    if (cmdUpper.length() > 3) {
      String args = cmdUpper.substring(3);
      args.trim();
      int sp = args.indexOf(' ');
      if (sp > 0) {
        String s1 = args.substring(0, sp);
        String s2 = args.substring(sp + 1);
        s1.trim();
        s2.trim();
        if (s1.startsWith("0X") && s2.startsWith("0X")) {
          i_start = (int)strtol(s1.c_str(), NULL, 16);
          i_end = (int)strtol(s2.c_str(), NULL, 16);
        }
      }
    }
    consultarMemoria(i_start, i_end);
  } else if (cmdUpper == "STATUS") {
    exibirStatus();
  } else {
    Serial.println(F("[ERRO] Comando do ambiente desconhecido."));
  }
}

// 8. MONTADOR ASSEMBLY (SINTAXE, COMENTÁRIOS E GERAÇÃO DA PALAVRA BINÁRIA) //

void processarLinhaAssembly(String linha) {
  // Tratamento de comentários (remover do ';' em diante)
  int posComent = linha.indexOf(';');
  if (posComent >= 0) {
    linha = linha.substring(0, posComent);
  }
  linha.trim();
  // Ignora linhas vazias ou puramente com comentários
  if (linha.length() == 0) return;

  if (ponteiro_carga >= MEM_SIZE) {
    Serial.println(F("[ERRO MONTADOR] Memoria de programa cheia (limite de 128 posicoes atingido)."));
    return;
  }

  uint16_t palavra_binaria = 0;
  if (validarEDecodificarAssembly(linha, palavra_binaria)) {
    MEM_PROG[ponteiro_carga] = palavra_binaria;
    CARREGADO[ponteiro_carga] = true;
    Serial.print(F("END: "));
    imprimirHexDoisDigitos(ponteiro_carga);
    Serial.print(F(" | PALAVRA BINARIA: "));
    imprimirBinario16Bits(palavra_binaria);
    Serial.print(F("

// 9. UNIDADE DE CONTROLE (UC) — CICLO DE BUSCA, DECODIFICAÇÃO E EXECUÇÃO POR BITS //

void executarCicloInstrucao() {
  if (PC >= MEM_SIZE || !CARREGADO[PC]) {
    Serial.print(F("[ERRO EXECUCAO] Acesso a posicao nao carregada da memoria em "));
    imprimirHexDoisDigitos(PC);
    Serial.println(F(". Interrompendo programa."));
    desligarTodasSaidas();
    modo_atual = MODO_ERRO;
    return;
  }
  
  // 1. BUSCA (Fetch)
  IR = MEM_PROG[PC];

  // 2. DECODIFICAÇÃO EXCLUSIVAMENTE POR BITS (Decode)
  uint8_t opcode = (IR >> 12) & 0x0F;
  uint8_t bomba = IR & 0x03;
  uint8_t dest = IR & 0x7F;
  uint8_t tempo = IR & 0x7F;
  uint8_t perc = IR & 0x7F;
  uint8_t bomba_cmp = (IR >> 10) & 0x03;
  String mnem_decodificado = "";
  uint8_t proximo_PC = PC + 1;
  bool alterou_PC = false;

// 3. EXECUÇÃO DA INSTRUÇÃO (Execute)
switch (opcode) {
  case OP_READ:
    mnem_decodificado = "READ " + String(bomba);
    NIVEL[bomba] = lerNivelPercentual(bomba);
    NIVEL_VALIDO[bomba] = true;
    ACC = NIVEL[bomba];
    break;
  case OP_ON:
    mnem_decodificado = "ON " + String(bomba);
    digitalWrite(PIN_BOMBA[bomba], HIGH);
    break;
  case OP_OFF:
    mnem_decodificado = "OFF " + String(bomba);
    digitalWrite(PIN_BOMBA[bomba], LOW);
    break;
  case OP_ALARM:
    mnem_decodificado = "ALARM " + String(bomba);
    ALARM_CORRETIVO[bomba] = true;
    atualizarBuzzer();
    Serial.print(F("[ALARME CORRETIVO ATIVADO] Bomba: "));
    Serial.println(bomba);
    break;
  case OP_LED:
    mnem_decodificado = "LED " + String(bomba);
    digitalWrite(PIN_PREV[bomba], HIGH);
    break;
  case OP_INFO: {
    mnem_decodificado = "INFO " + String(bomba);
    NIVEL[bomba] = lerNivelPercentual(bomba);
    NIVEL_VALIDO[bomba] = true;
    ACC = NIVEL[bomba];
    uint8_t digito = percentualParaDigito(NIVEL[bomba]);
    atualizarDisplay7Seg(digito);
    Serial.print(F("[INFO] Bomba: "));
    Serial.print(bomba);
    Serial.print(F(" | Nivel: "));
    Serial.print(NIVEL[bomba]);
    Serial.print(F("% | Digito Display: "));
    Serial.println(digito);
    break;
  }
  case OP_SILENCE:
    mnem_decodificado = "SILENCE " + String(bomba);
    ALARM_CORRETIVO[bomba] = false;
    atualizarBuzzer();
    break;
  case OP_LEDOFF:
    mnem_decodificado = "LEDOFF " + String(bomba);
    digitalWrite(PIN_PREV[bomba], LOW);
    break;
  case OP_CMP:
    mnem_decodificado = "CMP " + String(bomba_cmp) + ", " + String(perc);
    if (!NIVEL_VALIDO[bomba_cmp]) {
      Serial.println(F("[ERRO EXECUCAO] Tentativa de CMP sem leitura previa valida."));
      modo_atual = MODO_ERRO;
      desligarTodasSaidas();
      return;
    }
    // Cálculo ULA: NIVEL[x] - n (guardado em ACC)
    ACC = (int16_t)NIVEL[bomba_cmp] - (int16_t)perc;
    FLAG_L = (ACC < 0);
    FLAG_Z = (ACC == 0);
    FLAG_G = (ACC > 0);
    COMP_VALIDA = true;
    break;
  case OP_JMP:
    mnem_decodificado = "JMP ";
    proximo_PC = dest;
    alterou_PC = true;
    break;
  case OP_JL:
    mnem_decodificado = "JL ";
    if (!COMP_VALIDA) {
      Serial.println(F("[ERRO EXECUCAO] Desvio condicional sem comparacao valida previa."));
      modo_atual = MODO_ERRO;
      desligarTodasSaidas();
      return;
    }
    if (FLAG_L) {
      proximo_PC = dest;
      alterou_PC = true;
    }
    break;
  case OP_JE:
    mnem_decodificado = "JE ";
    if (!COMP_VALIDA) {
      Serial.println(F("[ERRO EXECUCAO] Desvio condicional sem comparacao valida previa."));
      modo_atual = MODO_ERRO;
      desligarTodasSaidas();
      return;
    }
    if (FLAG_Z) {
      proximo_PC = dest;
      alterou_PC = true;
    }
    break;
  case OP_JG:
    mnem_decodificado = "JG ";
    if (!COMP_VALIDA) {
      Serial.println(F("[ERRO EXECUCAO] Desvio condicional sem comparacao valida previa."));
      modo_atual = MODO_ERRO;
      desligarTodasSaidas();
      return;
    }
    if (FLAG_G) {
      proximo_PC = dest;
      alterou_PC = true;
    }
    break;
  case OP_WAIT:
    mnem_decodificado = "WAIT " + String(tempo);
    em_espera = true;
    tempo_espera_fim = millis() + ((unsigned long)tempo * 100UL);
    break;
  case OP_HALT:
    mnem_decodificado = "HALT";
    modo_atual = MODO_HALT;
    Serial.println(F("\n[SISTEMA] Programa encerrado normalmente por instrucao HALT. Saidas mantidas."));
    break;
  default:
    Serial.println(F("[ERRO EXECUCAO] Opcode invalido ou corrompido encontrado."));
    modo_atual = MODO_ERRO;
    desligarTodasSaidas();
    return;
}

// Exibição dos Registradores e Estados após cada Instrução (Seção 2.2.3)
Serial.print(F("EXEC: "));
imprimirHexDoisDigitos(PC);
Serial.print(F(" | IR: "));
imprimirBinario16Bits(IR);
Serial.print(F(" | DEC: "));
Serial.print(mnem_decodificado);
Serial.print(F(" | ACC: "));
Serial.print(ACC);
Serial.print(F(" | FLAGS [L:"));
Serial.print(FLAG_L);
Serial.print(F(" Z:"));
Serial.print(FLAG_Z);
Serial.print(F(" G:"));
Serial.print(FLAG_G);
Serial.print(F(" VAL:"));
Serial.print(COMP_VALIDA);
Serial.print(F("]"));

if (modo_atual != MODO_HALT && modo_atual != MODO_ERRO) {
  Serial.print(F(" | Prox PC: "));
  imprimirHexDoisDigitos(alterou_PC ? proximo_PC : PC + 1);
  if (!em_espera) PC = alterou_PC ? proximo_PC : PC + 1;
}
Serial.println();

// 10. INSPEÇÃO DE MEMÓRIA E STATUS (COMANDOS MEM E STATUS) // 

void consultarMemoria(int inicio, int fim) {
  if (inicio < 0) inicio = 0;
  if (fim >= MEM_SIZE) fim = MEM_SIZE - 1;
  if (inicio > fim) {
    Serial.println(F("[ERRO] Intervalo de memoria invalido."));
    return;
  }
  
  int ocupadas = 0;
  for (int i = 0; i < MEM_SIZE; i++)
    if (CARREGADO[i]) ocupadas++;

  Serial.println(F("\nEND | PALAVRA BINARIA"));
  Serial.println(F("-------+-----------------"));
  for (int i = inicio; i <= fim; i++) {
    imprimirHexDoisDigitos(i);
    Serial.print(F(" | "));
    imprimirBinario16Bits(MEM_PROG[i]);
    if (CARREGADO[i]) Serial.print(F(" *"));
    Serial.println();
  }
  Serial.print(F("Total de posicoes ocupadas: "));
  Serial.print(ocupadas);
  Serial.println(F(" de 128. (* = Carregado)\n"));
}

void exibirStatus() {
  Serial.println(F("\n====== STATUS DO PROCESSADOR SIMULADO ======"));
  Serial.print(F("Modo de Operacao: "));
  switch(modo_atual) {
    case MODO_IDLE: Serial.println(F("IDLE")); break;
    case MODO_LOAD: Serial.println(F("LOAD")); break;
    case MODO_RUN_STEP: Serial.println(F("RUN (PASSO A PASSO)")); break;
    case MODO_AUTO: Serial.println(F("AUTO (CONTINUO)")); break;
    case MODO_HALT: Serial.println(F("HALT (PARADO)")); break;
    case MODO_ERRO: Serial.println(F("ERRO")); break;
  }
  Serial.print(F("PC (Contador de Programa): "));
  imprimirHexDoisDigitos(PC);
  Serial.println();
  Serial.print(F("IR (Registrador Instrucao): "));
  imprimirBinario16Bits(IR);
  Serial.println();
  Serial.print(F("ACC (Acumulador): "));
  Serial.println(ACC);
  Serial.print(F("Flags de Comparacao: L="));
  Serial.print(FLAG_L);
  Serial.print(F(" Z="));
  Serial.print(FLAG_Z);
  Serial.print(F(" G="));
  Serial.print(FLAG_G);
  Serial.print(F(" | Comparacao Valida: "));
  Serial.println(COMP_VALIDA);

  for (int b = 1; b <= 3; b++) {
    Serial.print(F("Bomba "));
    Serial.print(b);
    Serial.print(F(" -> Nivel: "));
    Serial.print(NIVEL[b]);
    Serial.print(F("% | Valido: "));
    Serial.print(NIVEL_VALIDO[b]);
    Serial.print(F(" | LED Bomba: "));
    Serial.print(digitalRead(PIN_BOMBA[b]) ? "LIGADO" : "DESLIGADO");
    Serial.print(F(" | LED Prev: "));
    Serial.print(digitalRead(PIN_PREV[b]) ? "LIGADO" : "DESLIGADO");
    Serial.print(F(" | Alarme Corr: "));
    Serial.println(ALARM_CORRETIVO[b] ? "ATIVO" : "INATIVO");
  }
  Serial.println(F("=============================================\n"));
}
