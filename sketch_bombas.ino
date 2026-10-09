// 10. INSPEÇÃO DE MEMÓRIA E STATUS (COMANDOS MEM E STATUS) // ============================================================================
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
