#include <WiFi.h>
#include <WiFiManager.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <base64.h> 
#include <DHT.h>

#define DHT1_PIN  21 // ESP32 pin GPIO21 connected to DHT11 sensor
#define DHT2_PIN  19 
#define BUTTON_PIN 18 // GPIO18 connected to button
#define LED_PIN 2     // On-board LED
#define ANALOG_PIN 32
#define ANALOG_PIN_2 35
#define BUZZER_PIN 15

String serverName = "https://node.myhome474.fun/granja21";
  float humi  = 0;
  float tempC = 0;
  float humi2  = 0;
  float tempC2 = 0;
int buttonState = 0;

unsigned long lastTime = 0;
unsigned long lastCommandCheck = 0;
unsigned long commandCheckInterval = 10000;
int analog1 = 0,analog2 = 0;


int corrente_inst[300];
int zero = 0;
int diferenca = 0;
int leitura = 0;

float corrente_pico;
float corrente_eficaz;
float tensao_rms;
float tensao_pico;
float frequencia_sinal;
double maior_valor = 0;
double corrente_valor = 0;
unsigned long tempo_zero_inicio;  
unsigned long tempo_zero_final;  
unsigned long semi_periodo;  
unsigned long periodo_completo;  

bool alarmOn = false;


DHT dht1(DHT1_PIN, DHT11);
DHT dht2(DHT2_PIN, DHT11);
WiFiManager wifiManager;
WebServer server(80);

void setup() {
  Serial.begin(115200);

wifiManager.autoConnect("StationFarm");

  dht1.begin(); // initialize the DHT11 sensor
  dht2.begin();

  pinMode(BUTTON_PIN, INPUT_PULLDOWN);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW); 
  digitalWrite(BUZZER_PIN, HIGH); 
 analogReadResolution(12);

}

void loop() {
unsigned long now = millis();
 analog1 = analogRead(ANALOG_PIN);
 analog2 = analogRead(ANALOG_PIN_2);
if(!digitalRead(BUTTON_PIN)){
buttonState = 1; 
}

  if (now - lastTime >= 3000) {  
    readVoltage();
    readDhtSensors();
    sendSensorData();
    lastTime = now;
    Serial.print("analog2: ");
    Serial.println(analog2);
    }

  if ((now - lastCommandCheck) > commandCheckInterval) {
    checkForCommands();
    lastCommandCheck = now;
  }

if(tensao_rms >= 10 && alarmOn){
    startBuzzer(10, 1000, 1000); 
}

  handleBuzzer();
}


void sendSensorData() {
    if (WiFi.status() == WL_CONNECTED) {  // Verifica se o ESP32 está conectado ao WiFi
        HTTPClient http;
        http.begin(serverName.c_str());  // URL da API
        digitalWrite(LED_PIN, HIGH);
        // Definição do usuário e senha para autenticação Basic
        String auth = base64::encode("admin:415263");
        http.addHeader("Authorization", "Basic " + auth);
        http.addHeader("Content-Type", "application/json");
        
        // Criação do JSON com os dados dos sensores
        String payload = "{\"sensor1\":" + String(tempC) + ",\"sensor2\":" + String(humi) + ",\"sensor3\":" + String(tempC2) + ",\"sensor4\":" + String(humi2) + ",\"analog1\":" + String(analog1) + ",\"analog2\":" + String(analog2) +  ",\"tensao\":" + String(tensao_rms) + ",\"buttonState\":" + String(buttonState) + "}";
        int httpResponseCode = http.POST(payload);
        buttonState = 0;
        if (httpResponseCode > 0) {
            String response = http.getString();
            Serial.println(httpResponseCode);  // Código de resposta
            Serial.println(response);  // Resposta da API
        } else {
            Serial.print("Erro na solicitação POST: ");
            Serial.println(httpResponseCode);
        }

        http.end();  // Fecha a conexão
    }
    digitalWrite(LED_PIN, LOW);
    Serial.println(analog1);
}


void readDhtSensors(){

  humi  = dht1.readHumidity();
  tempC = dht1.readTemperature();

  humi2  = dht2.readHumidity();
  tempC2 = dht2.readTemperature();

  // check whether the reading is successful or not
  if ( isnan(tempC) || isnan(humi)) {
    Serial.println("Failed to read from DHT1 sensor!");
      humi  = 0;
      tempC = 0;
  } else {
    Serial.print("Humidity 1: ");
    Serial.print(humi);
    Serial.print("%");

    Serial.print("  |  ");

    Serial.print("Temperature 1: ");
    Serial.print(tempC);
    Serial.println("°C");
  }

  if ( isnan(tempC2) || isnan(humi2)) {
    Serial.println("Failed to read from DHT2 sensor!");
      humi2  = 0;
      tempC2 = 0;
  } else {
    Serial.print("Humidity 2: ");
    Serial.print(humi2);
    Serial.print("%");

    Serial.print("  |  ");

    Serial.print("Temperature 2: ");
    Serial.print(tempC2);
    Serial.println("°C");
  }

}

void readVoltage(){
  maior_valor = 0;
  int total_valor = 0;

  // Fazendo várias leituras para capturar o pico corretamente
  for (int i = 0; i < 300; i++) {
    corrente_inst[i] = analogRead(ANALOG_PIN_2);
  }

  // Encontrar o valor máximo
  for (int i = 0; i < 300; i++) {
    if (maior_valor < corrente_inst[i]) {
      maior_valor = corrente_inst[i];
    }
  }

  // Mapear o valor máximo para a tensão de pico
  tensao_pico = map(maior_valor, 1860, 2260, 0, 320);  
  tensao_rms = tensao_pico / 1.414;  // Usando a constante de 1.414 para senoidal

  // Imprimir resultados para verificação
  Serial.print("Maior Valor: ");
  Serial.println(maior_valor);
  Serial.print("Tensão de Pico: ");
  Serial.println(tensao_pico);
  Serial.print("Tensão de Rede Elétrica (RMS): ");
  Serial.println(tensao_rms);

}

void checkForCommands() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin("https://node.myhome474.fun/granja21/command");
        String auth = base64::encode("admin:415263");
        http.addHeader("Authorization", "Basic " + auth);
        http.addHeader("Content-Type", "application/json");

    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("Command response: " + response);  // Resposta da API
      if (response == "restart") {
        ESP.restart();
      }
      if (response == "alarmON") {
        alarmOn = true;
      }
      if (response == "alarmOFF") {
        alarmOn = false;
      }      
    } else {
      Serial.print("Erro na solicitação GET: ");
      Serial.println(httpResponseCode);
    }

    http.end();  // Fecha a conexão
  }
}
