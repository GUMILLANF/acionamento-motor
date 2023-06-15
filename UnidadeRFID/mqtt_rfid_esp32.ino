#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <MFRC522.h> // biblioteca responsavel pela comunicacao com o módulo RFID-RC522
#include <SPI.h> // biblioteca para comunicação do barramento SPI
#include <HTTPClient.h>
//#include <LiquidCrystal_I2C.h>

#define SS_PIN   21 // SDA
#define RST_PIN  22 // RST

#define SIZE_BUFFER     18
#define MAX_SIZE_BLOCK  16

// set LCD colunas e linhas
int lcdColumns = 16;
int lcdRows = 2;

//LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);


// definições do LED (Sempre usar 'const' para não correr risco de sobrescrever a variavel em tempo de execução)
const int WHITE_PIN = 25;
const int RED_PIN = 26;
const int WHITE2_PIN = 27;

const int BOTAO = 14;

// esse objet 'chave' é utilizado para autenticação
MFRC522::MIFARE_Key key;

// Definicoes pino modulo RC522
MFRC522 mfrc522(SS_PIN, RST_PIN);


const char* ssid = "loren"; // ssid wifi / ssid "iPhoneZe"
const char* password = "l@rensid"; // senha wifi / senha iPhoneZe : 12344321 / TI = "17x09y2019 / loren = l@rensid"
int chip_id = ESP.getEfuseMac();

const char* mqttServer = "mqtt.wvieira.com.br"; // mqtt server
const char* mqttUser  = "mosquitto"; // mqtt user
const char* mqttPass = "2022@13x09"; // mqtt senha

const String apiURL = "http://mqtt.wvieira.com.br:8080/v1/rfid";

WiFiClientSecure wifiClient; // Define uma instância de cliente com TLS Wifi para conexão do broker mqtt.
PubSubClient mqttClient(wifiClient); // Define uma instância mqtt.

HTTPClient httpClient;

int registrar = 0;

int lastState = HIGH;
int currentState = 0;

void setup() {
  pinMode(WHITE_PIN, OUTPUT);
  pinMode(RED_PIN, OUTPUT);
  pinMode(WHITE2_PIN, OUTPUT);

  pinMode(BOTAO, INPUT_PULLUP);

  digitalWrite(WHITE_PIN, LOW);
  digitalWrite(RED_PIN, LOW);
  digitalWrite(WHITE2_PIN, LOW);

  //lcd.init();
  //lcd.backlight();

  Serial.begin(115200);
  delay(100);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  uint32_t notConnectedCounter = 0;
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(WHITE_PIN, HIGH);

    delay(500);
    Serial.println("Connecting to WiFi..");

    notConnectedCounter++;
    if (notConnectedCounter > 50) {
      Serial.println("Reiniciando placa ... ");
      ESP.restart();
    }

    digitalWrite(WHITE_PIN, LOW);
  }
  Serial.println("Connected to the WiFi network");

  SPI.begin(); // Init SPI bus

  Serial.println("Aproxime o seu cartao do leitor ... ");


  // Inicia MFRC522
  mfrc522.PCD_Init();
  // Mensagens iniciais no serial monitor

  Serial.println();

  wifiClient.setInsecure();
  mqttClient.setServer("mqtt.wvieira.com.br", 8083);
  while (!mqttClient.connected()) {
    Serial.println("Connecting to MQTT...");

    mqttClient.setKeepAlive(600); // mantem conectado por 600 segundos
    if (mqttClient.connect("Esp32RegistradorLeitor", mqttUser , mqttPass)) {
      Serial.println("connected");
      
      for (int x = 0; x < 2; x++)
      {
        digitalWrite(WHITE_PIN, HIGH);
        delay(300);
        digitalWrite(RED_PIN, HIGH);
        delay(300);
        digitalWrite(WHITE2_PIN, HIGH);
        delay(300);

        digitalWrite(WHITE2_PIN, LOW);
        delay(300);
        digitalWrite(RED_PIN, LOW);
        delay(300);
        digitalWrite(WHITE_PIN, LOW);
        delay(300);
      }

      digitalWrite(WHITE_PIN, LOW);
      digitalWrite(WHITE2_PIN, LOW);
      digitalWrite(RED_PIN, LOW);

    } else {
      /*
               int state()
               -4 : MQTT_CONNECTION_TIMEOUT - the server didn't respond within the keepalive time
               -3 : MQTT_CONNECTION_LOST - the network connection was broken
               -2 : MQTT_CONNECT_FAILED - the network connection failed
               -1 : MQTT_DISCONNECTED - the client is disconnected cleanly
               0 : MQTT_CONNECTED - the client is connected
               1 : MQTT_CONNECT_BAD_PROTOCOL - the server doesn't support the requested version of MQTT
               2 : MQTT_CONNECT_BAD_CLIENT_ID - the server rejected the client identifier
               3 : MQTT_CONNECT_UNAVAILABLE - the server was unable to accept the connection
               4 : MQTT_CONNECT_BAD_CREDENTIALS - the username/password were rejected
               5 : MQTT_CONNECT_UNAUTHORIZED - the client was not authorized to connect
      */
      Serial.print("failed with state ");
      Serial.print(mqttClient.state());
      delay(2000);
    }
  }
}


void loop() {
  currentState = digitalRead(BOTAO);

  if (lastState == LOW && currentState == HIGH) {
    Serial.println("Foi pressionado");

    if (registrar == 1) {
      registrar = 0;
      digitalWrite(WHITE2_PIN, LOW);
    } else {
      registrar = 1;
      digitalWrite(WHITE2_PIN, HIGH);
    }

    Serial.print("Registrar agora é: ");
    Serial.print(registrar);
    Serial.println("");
  }

  lastState = currentState;

  // Aguarda a aproximacao do cartao
  if ( ! mfrc522.PICC_IsNewCardPresent())
  {
    return;
  }
  // Seleciona um dos cartoes
  if ( ! mfrc522.PICC_ReadCardSerial())
  {
    return;
  }

  String conteudo = "";
  byte letra;
  for (byte i = 0; i < mfrc522.uid.size; i++)
  {
    // Quando leitura for detectada, pisca o led.
    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(mfrc522.uid.uidByte[i], HEX);
    conteudo.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
    conteudo.concat(String(mfrc522.uid.uidByte[i], HEX));
  }

  if (conteudo.length() > 0) {
    conteudo.toUpperCase();
    Serial.println("Tag : " + String(conteudo));// leitura RFID Chaveiro ex: Tag : 8a 7e 1b b0
    piscaLedLeituraCartao();
    requisicao(conteudo);
  }

  // instrui o PICC quando no estado ACTIVE a ir para um estado de "parada"
  mfrc522.PICC_HaltA();
  // "stop" a encriptação do PCD, deve ser chamado após a comunicação com autenticação, caso contrário novas comunicações não poderão ser iniciadas
  mfrc522.PCD_StopCrypto1();
  // fim da leitura RF Id

  mqttClient.loop();

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    reconnectWiFi();
  }

  while (!mqttClient.connected()) {
    Serial.println("Re-connecting to MQTT...");

    mqttClient.setKeepAlive(60); // mantem conectado por 60 segundos
    if (mqttClient.connect("Esp32RegistradorLeitor", mqttUser , mqttPass)) {
      Serial.println("reconnected to mqtt");
    } else {
      Serial.print("failed with state ");
      Serial.print(mqttClient.state());
      delay(2000);
    }
  }
}

void requisicao(String conteudo) {
  // FAZER UMA VERIFICAÇÃO AQUI SE FOI LIDO UM CARTÃO ANTES DE FAZER SOLICITAÇÃO HTTP PARA CHECAR.

  httpClient.begin(apiURL.c_str());
  httpClient.addHeader("Content-Type", "text/json");
  String body = String(conteudo);

  int responseCode = 0;

  if (registrar == 1) {
    // GRAVAÇÃO
    responseCode = httpClient.PUT(body);
  } else {
    // LEITURA
    responseCode = httpClient.POST(body);
  }

  Serial.print("HTTP code: ");
  Serial.print(responseCode);
  Serial.println("");

  if (responseCode >= 200 && responseCode < 300) {
    // Tratar Códigos de sucesso, ...
    String payload = httpClient.getString();

    if (payload.length() > 0) {
      Serial.print("Payload: ");
      Serial.println(payload);

      if ( (payload == "ACTIVE") && (registrar == 0) ) { // faz o pub somente qdo no modo ACESSO "LEITURA" e se o retorno do cartao lido for ACTIVE
        Serial.println("pub no mqtt efetuado.. ");
        mqttClient.publish("topic/motor", "on");
      }
    }
  } else if (responseCode >= 400 && responseCode < 500) {
    // Tratar um Bad request, Forbidden, Unauthorized, ...
    Serial.println("Houve um erro na requisição!");
    piscaLedErro();
  } else {
    // Server error (500...)
    Serial.println("Erro desconhecido!");
    piscaLedErro();
  }
}

void piscaLedLeituraCartao() {
  for (int x = 0; x < 3; x++) {
    digitalWrite(WHITE_PIN, HIGH);
    delay(200);
    digitalWrite(WHITE_PIN, LOW);
  }
}

void piscaLedErro() {
  for (int x = 0; x < 2; x++)
  {
    digitalWrite(RED_PIN, HIGH);
    delay(500);
    digitalWrite(RED_PIN, LOW);
    delay(200);

    digitalWrite(RED_PIN, HIGH);
    delay(500);
    digitalWrite(RED_PIN, LOW);
    delay(200);
  }
  digitalWrite(RED_PIN, LOW);

}

void reconnectWiFi() {
  int tentativas = 0;

  // Enquanto o wifi não estiver conectado, executa o bloco repetidamente.
  while (WiFi.status() != WL_CONNECTED) {

    digitalWrite(WHITE_PIN, HIGH);

    if (tentativas == 30) {
      // Após as tentativas, reinicia o ESP.
      ESP.restart();
    }

    digitalWrite(WHITE_PIN, LOW);

    tentativas++;
    WiFi.reconnect();
    delay(1000);
    yield();
    //ESP.wdtFeed();
  }
}
