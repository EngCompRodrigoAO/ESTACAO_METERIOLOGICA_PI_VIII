#include <Arduino.h>
#include <WiFi.h>
#include <SPI.h>
#include <Wire.h>
#include "ThingSpeak.h"
#include <ArduinoUniqueID.h>
#include <time.h>
#include <BME280.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

//---------------------------------------------------------------------------------------------------- DEFINIÇÕES
#define SENSOR_VELOCIDADE_VENTO_DIGITAL 13
#define SENSOR_VELOCIDADE_VENTO_ANALOGICO 12
#define SENSOR_ORIENTACAO_VENTO_ANALOGICO 14
#define SENSOR_PLUVIOMETRICO_DIGITAL 27
#define SENSOR_SISMICO_DIGITAL 26
#define SENSOR_SISMICO_ANALOGICO 25
#define SENSOR_ULTRAVIOLETA 33
#define SENSOR_HUMIDADE_SOLO
#define SENSOR_QUALIDADE_AR_MQ135 32
#define REF_3V3 4
#define SEALEVELPRESSURE_HPA (1013.25)

//--------------------------------------------------------------------------------------------------   CONSTANTES
const char *apiWriteKey = "1OCLPVHNGHJUTZYH"; // Chave de gravação do ThingSpeak
const char *apiReadKey = "3HPJPRL5N7JV1MS3";  // Chave de leitura do ThingSpeak
const char *ssid = "FALANGE_SUPREMA";         // Nome da rede wifi ssid
const char *pass = "#kinecs#";                // Senha da rede wifi
const char *ntpServer = "pool.ntp.org";       // Servidor relogio mundial
const int daylightOffset_sec = -3600 * 3;     // Servidor relogio mundial segundos constantes em um dia
const long gmtOffset_sec = 0;                 // Sevidor relogio mundial GMT do Brasil

//--------------------------------------------------------------------------------------------- VARIAVEIS GLOBAIS
int rpm, CONTADOR_ATUALIZACAO_SERVER = 0, MES_ATUAL, MES_ANTERIOR, TEMPO_APRESENTA = 3000, interval = 1000, QUALIDADE_AR, NIVEL_UV, BIRUTA, PLUVIOMETRICO, UMIDADE_SOLO, UMIDADE, PRESSAO, ALTITUDE, uvLevel;
float VELOCIDADE_VENTO, TEMPERATURA, SISMICO;
char DIRECAO_VENTO_NOMECLATURA[16][4] = {"N", "NNE", "NE", "ENE", "E", "ESE", "SE", "SSE", "S", "SSW", "SW", "WSW", "W", "WNW", "NW", "NNW"}; // DIREÇÃO DO VENTO
char COND_UV[5][9] = {"BAIXO", "MODERADO", "ALTO", "ELEVADO", "EXTREMO"};                                                                     // NIVEIS DE RADIAÇÃO UV
volatile byte pulsos;
unsigned long timeold;
unsigned int pulsos_por_volta = 20; // Altere o numero de acordo com o disco encoder
String NUMERO_SERIE = "";

//-------------------------------------------------------------------------------------------------------- FUNÇÕES
void contador()
{
  pulsos++; // Incrementa contador
}

// função que faz uma média de leituras em umA determinadA porta e retorna a média
int averageAnalogRead(int pinToRead, byte numberOfReadings) // pinToRead= pino analogico que quer obter a média, numberOfReadings= numero de vezes que deve fazer a leitura
{
  unsigned int runningValue = 0;

  for (int x = 0; x < numberOfReadings; x++)
    runningValue += analogRead(pinToRead);
  runningValue /= numberOfReadings;

  return (runningValue);
}

// FUÇÃO DE MAPA E RETORNA VALOR TRATADO
float mapfloat(float x, float in_min, float in_max, float out_min, float out_max) // x= valor a ser tatado, in_min= valor minimo de entrada, in_max= valor maximo a ser lido, out_mim= valor minimo da saida, out_max= valor maximo da saida
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// FUNÇÃO DIREÇÃO DO VENTO
int DIRECAO_VENTO()
{
  unsigned int POSICAO = 0;
  POSICAO = mapfloat(analogRead(SENSOR_ORIENTACAO_VENTO_ANALOGICO), 0, 4095, 0, 333);

  if (POSICAO >= 0 && POSICAO < 23)
  {
    POSICAO = 0;
  }
  else
  {
    if (POSICAO >= 22 && POSICAO < 46)
    {
      POSICAO = 1;
    }
    else
    {
      if (POSICAO >= 46 && POSICAO < 68)
      {
        POSICAO = 3;
      }
      else
      {
        if (POSICAO >= 68 && POSICAO < 91)
        {
          POSICAO = 4;
        }
        else
        {
          if (POSICAO >= 91 && POSICAO < 113)
          {
            POSICAO = 5;
          }
          else
          {
            if (POSICAO >= 113 && POSICAO < 135)
            {
              POSICAO = 6;
            }
            else
            {
              if (POSICAO >= 135 && POSICAO < 158)
              {
                POSICAO = 7;
              }
              else
              {
                if (POSICAO >= 158 && POSICAO < 180)
                {
                  POSICAO = 8;
                }
                else
                {
                  if (POSICAO >= 180 && POSICAO < 203)
                  {
                    POSICAO = 9;
                  }
                  else
                  {
                    if (POSICAO >= 203 && POSICAO < 226)
                    {
                      POSICAO = 10;
                    }
                    else
                    {
                      if (POSICAO >= 226 && POSICAO < 248)
                      {
                        POSICAO = 11;
                      }
                      else
                      {
                        if (POSICAO >= 248 && POSICAO < 270)
                        {
                          POSICAO = 12;
                        }
                        else
                        {
                          if (POSICAO >= 270 && POSICAO < 293)
                          {
                            POSICAO = 13;
                          }
                          else
                          {
                            if (POSICAO >= 293 && POSICAO < 315)
                            {
                              POSICAO = 14;
                            }
                            else
                            {
                              if (POSICAO >= 315 && POSICAO < 337)
                              {
                                POSICAO = 15;
                              }
                            }
                          }
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }

  return (POSICAO);
}

// Cria a  função para zerar os dados constantes no servidor
void RESETA_SERVER()
{
  // Carrega as informações para serem enviadas em um lote somente para o servidor
  ThingSpeak.setField(1, 0); // Fluxo corrente
  ThingSpeak.setField(2, 0); // Total em mililitros
  ThingSpeak.setField(3, 0); // Total em litros
  ThingSpeak.setField(4, 0); // Nivel reservatorio
  ThingSpeak.setField(5, 0); // Nivel acumulador
  ThingSpeak.setField(6, 0); // Status da Bomba
  ThingSpeak.setField(7, 0); // Status Solenoide Potavel
  ThingSpeak.setField(8, 0); // Status Solenoide Descarte
  ThingSpeak.setStatus("ZERA SERVIDOR");
  ThingSpeak.writeFields(1, apiWriteKey); // Envia o lote de informações para o servidor
  Serial.println("SERVIDOR ZERADO.");
}

// Cria a função para pegar a hora pelo Servidor GMT mundial.
void PEGAR_HORA()
{
  String MES_TEMP = "";
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    Serial.println("Falha ao obter hora");
    return;
  }
  Serial.println(&timeinfo, "%A, %d, %B, %Y, %H, %m, %S");
  Serial.println(&timeinfo, "%d/%m/%Y");
  MES_TEMP = (&timeinfo, "%m");
  MES_ATUAL = MES_TEMP.toInt();
  if (MES_ANTERIOR < MES_ATUAL)
  {
    Serial.println("RESETANDO SERVIDOR PARA O PROXIMO MES");
    RESETA_SERVER();
    MES_ANTERIOR = MES_ATUAL;
    Serial.println("SERVIDOR FOI RESETADO.");
  }
}

WiFiClient client;
Adafruit_BME280 bme;
void setup()
{
  Serial.begin(9600);
  // CONFIGURA OS PINOS COMO ENTRADA E SAIDA
  pinMode(REF_3V3, INPUT);                         // Pino para monitorar a tensão de referência
  pinMode(SENSOR_VELOCIDADE_VENTO_DIGITAL, INPUT); // Pino do sensor Velocidade do vento como entrada
  pinMode(SENSOR_ULTRAVIOLETA, INPUT);             // Pino do sensor Ultravioleta como entrada
  bme.begin(0x76);                                 // Endereço sensor BME280 0x77 ou 0x76
  // Interrupcao 0 - pino digital 13
  // Aciona o contador a cada pulso
  attachInterrupt(SENSOR_VELOCIDADE_VENTO_DIGITAL, contador, RISING);
  pulsos = 0;
  rpm = 0;
  VELOCIDADE_VENTO = 0;
  timeold = 0;

  // UniqueIDdump(Serial);
  NUMERO_SERIE = "MCUDEVICE-";
  // Serial.print("UniqueID: ");
  for (size_t i = 0; i < UniqueIDsize; i++)
  {
    if (UniqueID[i] < 0x10)
      Serial.print("0");

    // Serial.print(UniqueID[i], HEX);
    NUMERO_SERIE += String(UniqueID[i], HEX);
    // Serial.print("");
  }
  Serial.println();

  NUMERO_SERIE.toUpperCase();
  Serial.println(NUMERO_SERIE);
  WiFi.mode(WIFI_STA);      // Coloca o WIFI do ESP32 em modo estação
  ThingSpeak.begin(client); // Inicializa a comunicação com o ThingSpeak

  /* FAZ A VERIFICAÇÃO SE TEM CONECTIVIDADE COM WIFI CASO TENHA PASSA A FRENTE CASO NÃO FAZ A CONEXÃO COM A REDE
     WIFI PRÉ ESTABELECIDA NA CONSTANTE SSID E PASS.*/
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.print("PROCURANDO WIFI.");
    delay(2000);
    Serial.print(".");
    delay(2000);
    Serial.print(".");
    delay(2000);
    Serial.print(".");
    delay(2000);
    Serial.print(".");
    delay(2000);
    Serial.println(".");
    delay(2000);
    Serial.print("!!WIFI ENCONTRADO!!");
    delay(2000);
    Serial.print("AGUARDE CONECTANDO..");
    Serial.println("AGUARDE CONECTANDO.");
    // digitalWrite(LED_WIFI, LOW);
    while (WiFi.status() != WL_CONNECTED)
    {
      WiFi.begin(ssid, pass);
      delay(5000);
    }
    Serial.print("WIFI CONECTADO.");
  }
}

void loop()
{
  TEMPERATURA=bme.readTemperature();
  PRESSAO=bme.readPressure();
  UMIDADE=bme.readHumidity();
  ALTITUDE = bme.readAltitude(SEALEVELPRESSURE_HPA);

  // Atualiza contador a cada segundo
  if (millis() - timeold >= 1000)
  {
    detachInterrupt(SENSOR_VELOCIDADE_VENTO_DIGITAL);                     // Desabilita interrupcao durante o calculo
    rpm = (60 * 1000 / pulsos_por_volta) / (millis() - timeold) * pulsos; // RPM do sensor
    VELOCIDADE_VENTO = ((((2 * 3.6) * 3, 14) * 1.3) * rpm) / 60, 0;       // converte RPM em Km/h
    timeold = millis();
    pulsos = 0;
    // Mostra o valores no serial monitor
    Serial.print("TEMPERATURA: ");
    Serial.print(TEMPERATURA);
    Serial.print(" °C");
    Serial.print(" | ");
    Serial.print("UMIDADE: ");
    Serial.print(UMIDADE);
    Serial.print(" %RH");
    Serial.print(" | ");
    Serial.print("PRESSÃO: ");
    Serial.print(PRESSAO);
    Serial.print(" hPa");
    Serial.print(" | ");
    Serial.print("ALTITUDE: ");
    Serial.print(ALTITUDE);
    Serial.print(" M");
    Serial.print(" | ");
    Serial.print("UV: ");
    Serial.print(NIVEL_UV);
    Serial.print(" - CONDIÇÃO: ");
    Serial.print(COND_UV[1]);
    Serial.print(" | ");
    Serial.print("ANEMOMETRO: ");
    Serial.print(VELOCIDADE_VENTO);
    Serial.print(" KM/h");
    Serial.print(" | ");
    Serial.print("BIRUTA: ");
    Serial.print(BIRUTA);
    Serial.print(" - DIREÇÃO: ");
    Serial.print(DIRECAO_VENTO_NOMECLATURA[DIRECAO_VENTO()]);
    Serial.print(" | ");
    Serial.print("PLUVIOMETRICO: ");
    Serial.print(PLUVIOMETRICO);
    Serial.print(" mm/M²");
    Serial.print(" | ");
    Serial.print("SISMICO: ");
    Serial.print(SISMICO);
    Serial.print(" °");
    Serial.print(" | ");
    Serial.print("UMIDADE SOLO: ");
    Serial.print(UMIDADE_SOLO);
    Serial.print(" %RH");
    Serial.print(" | ");
    Serial.print("Vento: ");
    Serial.print(VELOCIDADE_VENTO);
    Serial.print(" km/h");
    Serial.print(" | ");
    Serial.println("");
    attachInterrupt(SENSOR_VELOCIDADE_VENTO_DIGITAL, contador, RISING); // Habilita interrupcao
  }

  if (CONTADOR_ATUALIZACAO_SERVER == 1) // 3,2 para 1 segundos esta tem que ser do tipo int
  {
    // Carrega as informações para serem enviadas em um lote somente para o servidor
    ThingSpeak.setField(1, float(TEMPERATURA)); // Sensor 1 BME280 TEMPERATURA
    ThingSpeak.setField(2, UMIDADE);            // Sensor 2 BME280 UMIDADE
    ThingSpeak.setField(3, NIVEL_UV);           // Sensor 3 ML8511 UV
    ThingSpeak.setField(4, VELOCIDADE_VENTO);   // Sensor 4 ANEMOMETRO
    ThingSpeak.setField(5, BIRUTA);             // Sensor 5 BITUTA
    ThingSpeak.setField(6, PLUVIOMETRICO);      // Sensor 6 PLUVIOMETRICO
    ThingSpeak.setField(7, SISMICO);            // Sensor 7 SISMICO
    ThingSpeak.setField(8, UMIDADE_SOLO);       // Sensor 8 HUMIDADE SOLO
    ThingSpeak.setStatus(String(QUALIDADE_AR)); // SENSOR 9 QUALIDADE DO AR
    ThingSpeak.writeFields(1, apiWriteKey);     // Envia o lote de informações para o servidor
    CONTADOR_ATUALIZACAO_SERVER = 0;
  }
  /*else // rever esta parte para adequar ao novo projeto.
  {
      // Carrega a informação do servidor o total acumulada de mililitros
      totalMilliLitres = ThingSpeak.readFloatField(1753394, 2, apiReadKey);
      // Carrega a informação do servidor o total acumulado de litros.
      totalLitres = ThingSpeak.readFloatField(1753394, 3, apiReadKey);
  }*/
  int uvLevel = averageAnalogRead(SENSOR_ULTRAVIOLETA, 8);
  int refLevel = averageAnalogRead(REF_3V3, 8);
  float outputVoltagem = 3.3 / refLevel * uvLevel;                    // Use os 3,3V de alimentação no pin referência para um melhor acuidade do valor de saida do sensor
  float NIVEL_UV = mapfloat(outputVoltagem, 0.99, 2.8, 0.0, 15.0);    // Convert the voltage to a UV intensity level
}