const int buzzerPin = 9; // Pino conectado ao buzzer
unsigned long previousMillis = 0; // Marca o tempo anterior
unsigned long beepStartMillis = 0; // Marca o início de cada apito
bool buzzerOn = false; // Estado do buzzer
int beepsRemaining = 0; // Número de apitos restantes
int beepDuration = 0; // Duração de cada apito (em ms)
int pauseDuration = 0; // Pausa entre apitos (em ms)



// Função para iniciar o buzzer
void startBuzzer(int numBeeps, int duration, int pause) {
  beepsRemaining = numBeeps;
  beepDuration = duration;
  pauseDuration = pause;
  previousMillis = millis();
  buzzerOn = false; // O buzzer começa desligado
}

// Função para controlar o buzzer sem bloquear o código
void handleBuzzer() {
  unsigned long currentMillis = millis();

  if (beepsRemaining > 0) {
    if (buzzerOn && (currentMillis - beepStartMillis >= beepDuration)) {
      // Desliga o buzzer após o tempo de duração
      digitalWrite(BUZZER_PIN, HIGH);
      buzzerOn = false;
      previousMillis = currentMillis;
      beepsRemaining--; // Diminui o número de apitos restantes
    } else if (!buzzerOn && (currentMillis - previousMillis >= pauseDuration)) {
      // Liga o buzzer após a pausa
      digitalWrite(BUZZER_PIN, LOW);
      buzzerOn = true;
      beepStartMillis = currentMillis; // Marca o tempo do apito
    }
  } else {
    // Garante que o buzzer esteja desligado após o ciclo
    digitalWrite(BUZZER_PIN, HIGH);
  }
}
