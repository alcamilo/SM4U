#include <FirebaseESP32.h>
#include <Wire.h>
#include <HardwareSerial.h>
#include <ArduinoJson.h>                      //Biblioteca de manipulação de JSON Documents
#include <Adafruit_GFX.h>                     //Biblioteca do Display
#include <Adafruit_SSD1306.h>                 //Biblioteca do Display
#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFiManager.h>
#include <WiFi.h>
//#include <FreeRTOS.h>

#define   FIREBASE_HOST                "smartmed4u.firebaseio.com"
#define   FIREBASE_AUTH               "uvzu8AGHi1oN0u3J2bUCqu1qI0Xh5ctEJRc2u0gc"
#define   TEMPO_LEITURA_OXIMETRO      1000
#define   VELOCIDADE_SERIAL           9600
#define   TEMPODERESPOSTA             1000        //Tempo de resposta 1 s
#define   TEMPODERESPOSTA2            500         //Tempo de resposta 500 ms
#define   TEMPODERESPOSTA3            10000       //Tempo de resposta 10 s
#define   TEMPODERESPOSTA4            250         //Tempo de resposta 200 ms
#define   TEMPODERESPOSTA5            30000       //Tempo de resposta 30 s
#define   SCREEN_WIDTH                128         // OLED display width, in pixels
#define   SCREEN_HEIGHT               64          // OLED display height, in pixels
#define   ONEWIREBUS                  33          //Sensor de temperadura
#define   BT_UP                       25          //Porta do Botão UP
#define   BT_DOWN                     26          //Porta do Botão DOWN
#define   BT_ENTER                    27          //Porta do Botão ENTER
#define   S1_RX                       16
#define   S1_TX                       17

FirebaseData firebaseData;
HardwareSerial pox(2);
DynamicJsonDocument doc(200);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire);
OneWire onewire(ONEWIREBUS);
DallasTemperature sensor(&onewire);
WiFiManager wifiManager;

void initHW();
void leituraDisplay();
void leituraOximetro();
void leituraSensor(struct readings *r);
void leituraBotao();
void configModeCallback (WiFiManager *myWiFiManager);
void saveConfigCallback ();
void firebase();

void pulso()
{
  Serial.println("B:1");
}

enum { MENU = 0, PAGE_1, PAGE_2, PAGE_3};

unsigned long tempo = 0;
ulong tsPox = 0, tsDisplay = 0, tsSensor = 0, tsBotao = 0, tsFirebase = 0;
uint16_t xBPM = 0, Spo2 = 0;
uint8_t tela = MENU;
uint8_t valor = 1, enter = 0, posicao_atual = 0;
float_t temp = 0.00;
bool stMode = false;
String macAdd = WiFi.macAddress();

struct readings {
  int   bpm = xBPM;            // Beats per minute
  int   spo2 = Spo2;           // Oximetry in Percent    ( %)
  float temp = temp;           // Temperature in Celsius (°C)
} readings;

void setup() {
  initHW();
}

void loop() {
  if (millis() - tsPox >= TEMPODERESPOSTA)
  {
    leituraOximetro();
    if (xBPM >= 40 && tela == MENU)
    {
      //      drawlogo(1, 2);
    }
    tsPox = millis();
  }

  if (millis() - tsDisplay >= TEMPODERESPOSTA2)
  {
    leituraDisplay();
    tsDisplay = millis();
  }

  if (millis() - tsSensor >= TEMPODERESPOSTA5)
  {
    leituraSensor(&readings);
    tsSensor = millis();
    Serial.println(macAdd);
  }

  if (millis() - tsBotao >= TEMPODERESPOSTA2)
  {
    leituraBotao();
  }

  if((millis() - tsFirebase >= TEMPODERESPOSTA5) && Spo2 > 50 && temp > 10)
  {    
    readDataForFirebase(&readings);
    sendDataToFirebase(readings);    
    tsFirebase = millis();
  }
}

void initHW()
{

  /*Aqui começa a configuração do sensor de temperatura*/
  sensor.begin();
  sensor.requestTemperaturesByIndex(0);
  temp = sensor.getTempCByIndex(0);

  /*Configurações gerais*/
  Serial.begin(VELOCIDADE_SERIAL);
  Wire.begin();

  /*Aqui começa a configuração do display*/
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3c))
  {
    Serial.println("Erro ao iniciar o display");
    for (;;);
  } else
  {
    Serial.println("Display iniciado com sucesso");
  }
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.print("SmartMed4U");
  display.display();

  /*Aqui começa a configuração dos botões*/
  pinMode(BT_DOWN, INPUT_PULLUP);
  pinMode(BT_UP, INPUT_PULLUP);
  pinMode(BT_ENTER, INPUT_PULLUP);

  /*configuração do WifiManager*/
  wifiManager.autoConnect("SmartMed4U");

  /*Configuração do firebase*/
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);

  /*Aqui começa a configuração do Oxímetro*/
  
  pox.begin(9600, SERIAL_8N1, S1_RX, S1_TX);

}

void leituraOximetro() {
    Serial.println(pox.available());
  if(pox.available() > 0)
  {
    DeserializationError erro = deserializeJson(doc, pox);
  
    if(erro == DeserializationError::Ok)
    {
      Serial.print("Sensor: ");
      Serial.print(doc["sensor"].as<String>());
      Serial.print("/  BPM: ");
      xBPM = doc["bpm"].as<int16_t>();
      Serial.print(xBPM);
      Serial.print("/  Spo2: ");
      Spo2 = doc["spo2"].as<int16_t>();
      Serial.println(Spo2);
    }
    else
    {
      Serial.print("deserializantionError retornou: ");
      Serial.println(erro.c_str());
      xBPM = 0;
      Spo2 = 0;
      if(tela == MENU)
      {
        //drawlogo(1,3);
      }
      while (pox.available()>0)
      {
        pox.read();
      }
    }
  }else
  {      
    if(tela == MENU)
    {
      //drawlogo(1,3);
    }
    Serial.println("Pox is no avalaible");
    //pox.end();
    //pox.begin(9600);
  }
}
void leituraDisplay()
{
  display.clearDisplay();
  switch (tela)
  {
    case MENU:
      {
        if (enter == 1)
        {

          tela = PAGE_1;
          enter = 0;
          posicao_atual = 0;
        }
        display.clearDisplay();
        display.setCursor(0, 0);
        display.setTextSize(2);
        display.print(xBPM);
        display.print("BPM");
        display.setCursor(0, 18);
        display.print(Spo2);
        display.print("%");
        display.setCursor(0, 36);
        display.printf("%.1f C", temp);
        display.drawCircle(55, 37, 2, WHITE);
        display.setCursor(58, 36);
        display.print("C");
        display.setTextSize(1);
        display.setCursor(0, 54);
        display.print("PRESS ENTER FOR MENU");
        display.display();
        break;
      }
    case PAGE_1:
      {
        display.clearDisplay();
        display.setTextSize(2);
        display.setCursor(13, 5);
        display.print("INICIO");
        display.setCursor(1, 22);
        display.print("STATUS WIFI");
        display.setCursor(10, 39);
        display.print("RECONNECT");

        if (posicao_atual == 0)
        {
          Serial.println("Dentro do Item INICIO");
          display.drawRoundRect(0, 4, 128, 16, 5, 1);
        } else if (posicao_atual == 1)
        {
          Serial.println("Dentro do Item STATUS WIFI");
          display.drawRoundRect(0, 21, 128, 16, 5, 1);
        } else if (posicao_atual == 2)
        {
          Serial.println("Dentro do Item RECONECT");
          display.drawRoundRect(0, 38, 128, 16, 5, 1);
        }

        display.display();
        Serial.println("valor ");
        Serial.print(valor);
        Serial.println("Posicao");
        Serial.print(posicao_atual);

        if (enter == 0 && valor == 2 && posicao_atual < 2)
        {
          Serial.println("dentro da soma");
          posicao_atual = posicao_atual + 1;
          Serial.println(posicao_atual);
          valor = 1;
        }
        if (enter == 0 && valor == 0 && posicao_atual > 0)
        {
          Serial.println("Dentro da subtracao");
          posicao_atual = posicao_atual - 1;
          valor = 1;
        }
        if (enter == 1 && posicao_atual == 0)
        {
          tela = MENU;
          enter = 0;
          posicao_atual = 0;
        }
        if (enter == 1 && posicao_atual == 1)
        {
          tela = PAGE_2;
          enter = 0;
          posicao_atual = 0;
        }
        if (enter == 1 && posicao_atual == 2)
        {
          tela = PAGE_3;
          enter = 0;
          posicao_atual = 0;
        }
        break;
      }
    case PAGE_2:
      {
        if (WiFi.isConnected())
        {
          display.clearDisplay();
          display.setCursor(0, 0);
          display.setTextSize(1);
          display.print("CONECTADO COMO");
          display.setCursor(0, 15);
          display.setTextSize(1);
          display.print(WiFi.SSID());
          display.display();
        } else
        {
          display.clearDisplay();
          display.setCursor(10, 10);
          display.setTextSize(2);
          display.print("Não Conectado");
          display.setCursor(10, 15);
          display.display();
          if (stMode)
          {
            display.setTextSize(1);
            display.print("Entrar no endereço:");
            display.setCursor(0, 20);
            display.print(WiFi.softAPIP());
            display.display();
          } else
          {
            display.setTextSize(1);
            display.setCursor(10, 10);
            display.print("Entar no modo de RECONNECT");
            display.display();
          }
        }

        if (enter == 1)
        {
          tela = MENU;
          enter = 0;
        }
        break;
      }
    case PAGE_3:
      {
        display.clearDisplay();
        display.setCursor(0, 0);
        display.setTextSize(1);
        display.print("MODO DE CONFIGURACAO");
        display.setCursor(0, 9);
        display.print("ATIVADO ");
        display.setCursor(0, 18);
        display.print("CONECTE NA REDE");
        display.setCursor(0, 27);
        display.print("SmartMed4U");
        display.setCursor(0, 36);
        display.print("NO IP");
        display.setCursor(0, 45);
        display.print("192.168.4.1");
        display.display();

        if (enter == 1)
        {
          WiFiManager wifiManager;
          if (!wifiManager.startConfigPortal("SmartMed4U") )
          {
            Serial.println("Falha ao conectar");
            delay(2000);
            ESP.restart();
          }
          tela = MENU;
          enter = 0;
        }
        break;
      }
    default:
      break;
  }
}

void leituraBotao()
{

  if (!digitalRead(BT_ENTER))
  {
    enter = 1;
    tsBotao = millis();
  } else if (!digitalRead(BT_UP))
  {
    valor = 2;
    tsBotao = millis();
  } else if (!digitalRead(BT_DOWN))
  {
    valor = 0;
    tsBotao = millis();
  }
}

void leituraSensor(struct readings *r)
{
  sensor.requestTemperaturesByIndex(0);
  temp = sensor.getTempCByIndex(0);
  Serial.print(temp);
  Serial.println("°C");
  readings.temp = temp;
}

void configModeCallback (WiFiManager *myWiFiManager)
{
  Serial.println("Entrou no modo de configuração");
  Serial.println(WiFi.softAPIP()); //imprime o IP do AP
  Serial.println(myWiFiManager->getConfigPortalSSID()); //imprime o SSID criado da rede
  stMode = true;
}

void saveConfigCallback ()
{
  Serial.println("Rede Salva");
  stMode = false;
}

void sendDataToFirebase(struct readings r)
{
  if (!Firebase.pushInt(firebaseData, macAdd + "/" + "BPM", r.bpm))
  {
    Serial.print("[ERROR] pushing BPM failed:");
  } else
  {
    Serial.print("Sending BPM");
  }

  if (!Firebase.pushInt(firebaseData, macAdd + "/" + "SpO2", r.spo2))
  {
    Serial.print("[ERROR] pushing SpO2 failed:");
  } else
  {
    Serial.print("Sending SpO2");
  }
  if (!Firebase.pushInt(firebaseData, macAdd + "/" + "Temperatura", r.temp))
  {
    Serial.println("[ERROR] pushing Temp failed:");
  } else
  {
    Serial.println("Sending Temp");
  }
}
void readDataForFirebase(struct readings *r)
{
  readings.bpm = xBPM;
  readings.spo2 = Spo2;
  readings.temp = temp;
}
