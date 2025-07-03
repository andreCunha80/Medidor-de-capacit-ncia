#define CAP_PIN 34
#define CHARGE_PIN 25

const int threshold = 2580;
const int R = 100000;
const unsigned long TIMEOUT = 100000;
const int numReadings = 5;

const float knownCapacitance_nF = 470.0;

float correctionFactor = 1.0;

void setup() {
  Serial.begin(115200);
  pinMode(CHARGE_PIN, OUTPUT);
  digitalWrite(CHARGE_PIN, LOW);
  delay(1000);

  Serial.println("=== Medidor de Capacitancia com Diagnostico ===");
  Serial.print("Conecte capacitor de ");
  Serial.print(knownCapacitance_nF);
  Serial.println(" nF para calibracao...");
  delay(4000);

  correctionFactor = calibrate();
  Serial.print("Fator de correcao: ");
  Serial.println(correctionFactor, 4);
  Serial.println("Pronto para medir outros capacitores.");
  delay(2000);
}

void loop() {
  float medidas[numReadings];
  float soma = 0;
  int validas = 0;

  for (int i = 0; i < numReadings; i++) {
    float val = medirCapacitanciaCorrigida();

    if (val > 0) {
      medidas[validas++] = val;
      soma += val;
    } else {
      Serial.println("❌ Medicao invalida (timeout). Verifique se o capacitor está aberto ou desconectado.");
      break;
    }

    delay(100);
  }

  if (validas == numReadings) {
    float media = soma / validas;

    // Cálculo de desvio simples para detectar instabilidade
    float desvio = 0;
    for (int i = 0; i < validas; i++) {
      desvio += abs(medidas[i] - media);
    }
    desvio /= validas;

    Serial.print("📏 Capacitancia: ");
    Serial.print(media, 2);
    Serial.println(" nF");

    // Diagnóstico automático:
    if (media < 1.0) {
      Serial.println("⚠️ Possível curto-circuito no capacitor.");
    } else if (media > 2000.0) {
      Serial.println("⚠️ Valor muito alto. Fora da faixa esperada.");
    }

    if (desvio > (0.1 * media)) {
      Serial.println("⚠️ Medidas instáveis. Possível fuga ou mau contato.");
    }
  }

  delay(3000);
}

float medirCapacitanciaCorrigida() {
  digitalWrite(CHARGE_PIN, LOW);
  delay(200);

  unsigned long start = micros();
  digitalWrite(CHARGE_PIN, HIGH);

  while (analogRead(CAP_PIN) < threshold) {
    if ((micros() - start) > TIMEOUT) {
      digitalWrite(CHARGE_PIN, LOW);
      return -1.0;
    }
  }

  unsigned long elapsed = micros() - start;
  digitalWrite(CHARGE_PIN, LOW);

  float t_sec = elapsed * 1e-6;
  float cap_nF = (t_sec / R) * 1e9;
  return cap_nF * correctionFactor;
}

float calibrate() {
  float total = 0;
  int valid = 0;

  for (int i = 0; i < numReadings; i++) {
    digitalWrite(CHARGE_PIN, LOW);
    delay(200);

    unsigned long start = micros();
    digitalWrite(CHARGE_PIN, HIGH);

    while (analogRead(CAP_PIN) < threshold) {
      if ((micros() - start) > TIMEOUT) {
        digitalWrite(CHARGE_PIN, LOW);
        return 1.0;
      }
    }

    unsigned long elapsed = micros() - start;
    digitalWrite(CHARGE_PIN, LOW);

    float t_sec = elapsed * 1e-6;
    float cap_nF = (t_sec / R) * 1e9;

    total += cap_nF;
    valid++;
    delay(100);
  }

  float media = total / valid;
  Serial.print("Valor medido: ");
  Serial.print(media, 2);
  Serial.println(" nF");

  return knownCapacitance_nF / media;
}
