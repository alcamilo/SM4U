#include <FirebaseESP32.h>
#include <Wire.h>
//#include <MAX30100_PulseOximeter.h>         //Biblioteca apenas para o MAX30100
#include "MAX30105.h"                         //Biblioteca para o MAX30102 e MAX30105
#include "spo2_algorithm.h"                   //Biblioteca para o MAX30102 e MAX30105
#include "heartRate.h"                        //Biblioteca para o MAX30102 e MAX30105
#include <Adafruit_GFX.h>                     //Biblioteca do Display 128x64
#include <Adafruit_SSD1306.h>                 //Biblioteca do Display 128x64
#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFiManager.h>


#define   FIREBASE_HOT                "smartmed4u.firebaseio.com"
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
#define   ONEWIREBUS                  32          //Sensor de temperadura
#define   BT_UP                       25          //Porta do Botão UP
#define   BT_DOWN                     26          //Porta do Botão DOWN
#define   BT_ENTER                    27          //Porta do Botão ENTER
#define   I2C_SDA                     21
#define   I2C_SCL                     22
#define RATE_SIZE 4 

FirebaseData firebasedata;
//PulseOximeter pox;                              //Biblioteca apenas para o MAX30100
MAX30105  pox;                                    //Biblioteca para o MAX30102 e MAX30105
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire1);
OneWire onewire(ONEWIREBUS);
DallasTemperature sensor(&onewire);
WiFiManager wifiManager;

enum { MENU = 0, PAGE_1, PAGE_2, PAGE_3};
struct readings {
  int   bpm;            // Beats per minute
  int   spo2;           // Oximetry in Percent    ( %)
  float temp;           // Temperature in Celsius (°C)
} readings;

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

unsigned long tempo = 0;
ulong tsPox = 0, tsDisplay = 0, tsSensor = 0, tsBotao = 0, tsFirebase = 0;
//uint16_t xBPM = 0, Spo2 = 0;
uint8_t tela = MENU;
uint8_t valor = 1, enter = 0, posicao_atual = 0;
float_t temp = 0.00;
bool stMode = false;

float BPM;
int beatAvg;
byte rates[RATE_SIZE]; //Array of heart rates
byte rateSpot = 0;
long lastBeat = 0; //Time at which the last beat occurred
int32_t bufferLength; //data length
int32_t spo2; //SPO2 value
int8_t validSPO2; //indicator to show if the SPO2 calculation is valid
int32_t heartRate; //heart rate value
int8_t validHeartRate; //indicator to show if the heart rate calculation is valid
uint32_t irBuffer[100]; //infrared LED sensor data
uint32_t redBuffer[100];  //red LED sensor data

void setup() {
  initHW();
}

void loop() {
//  pox.update();
//  if (millis() - tsPox >= TEMPODERESPOSTA)
//  {
//    leituraOximetro();
//    if (xBPM >= 40 && tela == MENU)
//    {
//      //      drawlogo(1, 2);
//    }
//    tsPox = millis();
//  }
//
//  if (millis() - tsDisplay >= TEMPODERESPOSTA2)
//  {
//    leituraDisplay();
//    tsDisplay = millis();
//  }
//
//  if (millis() - tsSensor >= TEMPODERESPOSTA5)
//  {
//    pox.shutdown();
//    leituraSensor(&readings);
//    tsSensor = millis();
//    pox.resume();
//  }
//
//   if(millis() - tsBotao >= TEMPODERESPOSTA2)
//  {
//    leituraBotao();
//  }
}

void initHW()
{

  /*Aqui começa a configuração do sensor de temperatura*/
  sensor.begin();
  sensor.requestTemperaturesByIndex(0);
  temp = sensor.getTempCByIndex(0);

  /*Configurações gerais*/
  Serial.begin(VELOCIDADE_SERIAL);
  //Wire.begin();
  if(Wire.begin(I2C_SDA,I2C_SCL))
  {
    Serial.println("Wire Begin OK");
  }
  if (!pox.begin(Wire, I2C_SPEED_FAST,0x57))
   {
    Serial.println("Falha na Inicializacao do Sensor");
    for(;;);
    }
    else 
    {
        Serial.println("Comunicacao Realizada com Sucesso!");
    }

  /*Configuração oximetro*/
  pox.setup(); //Configure sensor default settings
  pox.setPulseAmplitudeRed(0x0A); //Turn Red LED to low to indicate sensor is running
  pox.setPulseAmplitudeGreen(0); //Turn off Green LED
 
  /*Aqui começa a configuração do display*/
  Wire1.begin(I2C_SDA, I2C_SCL);
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

   /*Aqui começa a configuração do Oxímetro*/
//  if (!pox.begin())
//  {
//    Serial.println("Erro ao inicializar o Sensor");
//    for (;;);
//  }
//  pox.setOnBeatDetectedCallback(pulso);
//  pox.setIRLedCurrent(MAX30100_LED_CURR_30_6MA);

}

void leituraOximetro() {
    bufferLength = 100; 
  long irValue = pox.getIR();
  for (byte i = 0 ; i < bufferLength ; i++)
  {
    while (pox.available() == false) //do we have new data?
      pox.check(); //Check the sensor for new data

    redBuffer[i] = pox.getRed();
    irBuffer[i] = pox.getIR();
    pox.nextSample(); //We're finished with this sample so move to next sample
  }
   //calculate heart rate and SpO2 after first 100 samples (first 4 seconds of samples)
  //maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);
  
  while (1)
  { 
    if (checkForBeat(irValue) == true)
    {
      //We sensed a beat!
      long delta = millis() - lastBeat;
      lastBeat = millis();

      BPM = 60 / (delta / 1000.0);

      if (BPM < 255 && BPM > 20)
      {
        rates[rateSpot++] = (byte)BPM; //Store this reading in the array
        rateSpot %= RATE_SIZE; //Wrap variable

        //Take average of readings
        beatAvg = 0;
        for (byte x = 0 ; x < RATE_SIZE ; x++)
        beatAvg += rates[x];
        beatAvg /= RATE_SIZE;
      }
    }

    if (irValue < 50000)
    {
      Serial.println(" No finger?");
    }else
    {
      Serial.print(", BPM=");
      Serial.print(BPM);
    }

    //dumping the first 25 sets of samples in the memory and shift the last 75 sets of samples to the top
    for (byte i = 25; i < 100; i++)
    {
      redBuffer[i - 25] = redBuffer[i];
      irBuffer[i - 25] = irBuffer[i];
    }

    //take 25 sets of samples before calculating the heart rate.
    for (byte i = 75; i < 100; i++)
    {
      while (pox.available() == false) //do we have new data?
        pox.check(); //Check the sensor for new data


      redBuffer[i] = pox.getRed();
      irBuffer[i] = pox.getIR();
      pox.nextSample(); //We're finished with this sample so move to next sample

      //send samples and calculation result to terminal program through UART

      Serial.print(F(", SPO2="));
      Serial.print(spo2, DEC);

      Serial.print(F(", SPO2Valid="));
      Serial.println(validSPO2, DEC);
    }
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
        display.print(BPM);
        display.print("BPM");
        display.setCursor(0, 18);
        display.print(spo2);
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
