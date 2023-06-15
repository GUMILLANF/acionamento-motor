#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <LiquidCrystal_I2C.h>

#define LED_BR  12
#define LED_VM  32

// set LCD colunas e linhas
int lcdColumns = 16;
int lcdRows = 2;
String flagTemp = "off";

LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);

// definições do LED RGB (Sempre usar 'const' para não correr risco de sobrescrever a variavel em tempo de execução)

const int RED_PIN   = 25;
const int GREEN_PIN = 26;
const int BLUE_PIN  = 27;

const int RELE_PIN = 15; // Rele

const int TEMP_PIN = 34 ; // Sensor lm35

const char* ssid = "loren"; // ssid
const char* password = "l@rensid"; // // senha wifi /  senha: lore: l@rensid,  iPhoneZe : 12344321 / TI = "17x09y2019"
int chip_id = ESP.getEfuseMac();

const char* mqttServer = "mqtt.wvieira.com.br"; // mqtt server
const char* mqttUser  = "mosquitto"; // mqtt user
const char* mqttPass = "2022@13x09"; // mqtt senha

float Celsius;
int sensorValue;

WiFiClientSecure wifiClient; // Define uma instância de cliente com TLS Wifi para conexão do broker mqtt.
PubSubClient mqttClient(wifiClient); // Define uma instância mqtt.

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);

  String status = "";
  for (int i = 0; i < length; i++) {
    status += ((char)payload[i]);
  }

  Serial.println(status.c_str());

  if (status == "on") {
    if (digitalRead(15) == HIGH) {
      if (Celsius < 24.0) {
        digitalWrite(15, LOW);
        digitalWrite(LED_BR, HIGH);
        digitalWrite(LED_VM, LOW);
        mqttClient.publish("topic/alertatemp", "off"); 
      }

    } else {
      digitalWrite(15, HIGH);
      digitalWrite(LED_BR, LOW);
      digitalWrite(LED_VM, HIGH);
    }
  }

  if (status == "off") {
      digitalWrite(15, HIGH);
      digitalWrite(LED_BR, LOW);
      digitalWrite(LED_VM, HIGH);
  }
  
  status = "";
}

void setup() {
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);

  pinMode(LED_BR, OUTPUT);
  pinMode(LED_VM, OUTPUT);

  pinMode(15, OUTPUT);
  pinMode(TEMP_PIN, INPUT); // SENSOR MOVIMENTO

  digitalWrite(15, HIGH); // DESLIGA RELE

  lcd.init();
  lcd.backlight();

  Serial.begin(115200);
  delay(100);

  WiFi.begin(ssid, password);

  uint32_t notConnectedCounter = 0;
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(RED_PIN, HIGH);
    digitalWrite(GREEN_PIN, LOW);
    digitalWrite(BLUE_PIN, LOW);

    delay(500);
    //Serial.println("Connecting to WiFi..");
    lcd.setCursor(0, 0);
    lcd.print("Conectando-se ao ");
    lcd.setCursor(0, 1);
    lcd.print("   WiFi ");
    delay(1000);
    lcd.clear();
    notConnectedCounter++;
    if (notConnectedCounter > 50) {
      Serial.println("Reiniciando placa ... ");
      ESP.restart();
    }
  }
  //Serial.println("Connected to the WiFi network");
  lcd.setCursor(0, 0);
  lcd.print("Conectado");
  lcd.setCursor(1, 1);
  lcd.print("ao WiFi ");
  delay(1000);
  lcd.clear();

  Serial.println(WiFi.localIP());
  digitalWrite(RED_PIN, LOW);
  digitalWrite(GREEN_PIN, LOW);
  digitalWrite(BLUE_PIN, HIGH);

  digitalWrite(LED_BR, LOW);

  wifiClient.setInsecure();

  mqttClient.setServer("mqtt.wvieira.com.br", 8083);
  mqttClient.setCallback(callback);

  while (!mqttClient.connected()) {
    //Serial.println("Connecting to MQTT...");
    lcd.setCursor(0, 0);
    lcd.print("Conectando se");
    lcd.setCursor(0, 1);
    lcd.print(" ao MQTT ");
    delay(1000);
    lcd.clear();

    mqttClient.setKeepAlive(60); // mantem conectado por 60 segundos

    if (mqttClient.connect(String(chip_id).c_str(), mqttUser , mqttPass)) {
      Serial.println("connected");
      digitalWrite(RED_PIN, LOW);
      digitalWrite(GREEN_PIN, HIGH);
      digitalWrite(BLUE_PIN, HIGH);
    } else {
      Serial.print("failed with state ");
      Serial.print(mqttClient.state());
      delay(2000);
    }
  }

  mqttClient.subscribe("topic/motor");
}


void loop() {
  GetTemp();
  if (Celsius < 24.0) {
    lcd.setCursor(0, 0);
    lcd.print("Celsius : ");
    lcd.setCursor(0, 1);
    lcd.print(Celsius);
    delay(1000);
    lcd.clear();
    if (flagTemp == "on") {
       mqttClient.publish("topic/alertatemp", "off");
       flagTemp = "off";
    }
    
  } else {
    //Serial.println("Aumento temperatura ... ");
    lcd.setCursor(0, 0);
    lcd.print("ALERTA TEMP :");
    lcd.setCursor(0, 1);
    lcd.print(Celsius);
    delay(1000);
    lcd.clear();
    if (digitalRead(RELE_PIN) == LOW) {
      digitalWrite(RELE_PIN, HIGH);
      digitalWrite(LED_BR, LOW);
      digitalWrite(LED_VM, HIGH);
      mqttClient.publish("topic/motor", "off");
      mqttClient.publish("topic/alertatemp", "on"); 
      flagTemp = "on";
    }
  }


  mqttClient.loop();

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    reconnectWiFi();
  }

  while (!mqttClient.connected()) {
    Serial.println("Re-connecting to MQTT...");


    mqttClient.setKeepAlive(600); // mantem conectado por 600 segundos
    if (mqttClient.connect(String(chip_id).c_str(), mqttUser , mqttPass)) {
      Serial.println("reconnected to mqtt");
    } else {
      Serial.print("failed with state ");
      Serial.print(mqttClient.state());
      delay(2000);
    }
  }

  if (mqttClient.connected()) {
    // Enquanto conectado no MQTT
    digitalWrite(RED_PIN, LOW);
    digitalWrite(GREEN_PIN, HIGH);
    digitalWrite(BLUE_PIN, HIGH);
  }
}

void reconnectWiFi() {
  int tentativas = 0;

  // Enquanto o wifi não estiver conectado, executa o bloco repetidamente.
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(RED_PIN, HIGH);
    digitalWrite(GREEN_PIN, LOW);
    digitalWrite(BLUE_PIN, LOW);

    if (tentativas == 30) {
      // Após as tentativas, reinicia o ESP.
      ESP.restart();
    }

    tentativas++;
    WiFi.reconnect();
    delay(1000);
    yield();
    //ESP.wdtFeed();
  }
}

void GetTemp()
{
  sensorValue = analogRead(TEMP_PIN); // read the sensor

  Celsius = (((float(sensorValue) / 4096) * 5) * 100); // converte 10mv/C para Celcius
  //Kelvin = Celsius + 273.15; // convert to Kelvin
  //Fahrenheit = (Celsius * 1.8) +32;  // convert to Fahrenheit
}
