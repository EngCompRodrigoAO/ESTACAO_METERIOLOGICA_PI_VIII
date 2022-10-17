#include <Arduino.h>
//--------------------------------------------------------------------------------------------------------------------------------------- DEFINIÇÕES
#define SENSOR_VELOCIDADE_VENTO_DIGITAL 13
#define SENSOR_VELOCIDADE_VENTO_ANALOGICO 12
#define SENSOR_ORIENTACAO_VENTO_ANALOGICO 14
#define SENSOR_PLUVIOMETRICO_DIGITAL 27
#define SENSOR_SISMICO_DIGITAL 26
#define SENSOR_SISMICO_ANALOGICO 25
#define SENSOR_ULTRAVIOLETA 33
//#define SENSOR_BARROMETRICO
#define SENSOR_QUALIDADE_AR_MQ135 32
#define REF_3V3 4

int rpm;
float VELOCIDADE_VENTO;
volatile byte pulsos;
unsigned long timeold;

unsigned int pulsos_por_volta = 20; // Altere o numero de acordo com o disco encoder

void contador()
{
  pulsos++; // Incrementa contador
}

//Cria a funçãoque faz uma média de leituras em um determinado pino e retorna a média
int averageAnalogRead(int pinToRead, byte numberOfReadings) // pinToRead= pino analogico que quer obter a média, numberOfReadings= numero de vezes que deve fazer a leitura
{
   unsigned int runningValue = 0;

  for (int x = 0; x < numberOfReadings; x++)
    runningValue += analogRead(pinToRead);
  runningValue /= numberOfReadings;

  return (runningValue);
}

//cria uma função de MAPA e retorna o valor tratado
float mapfloat(float x, float in_min, float in_max, float out_min, float out_max)//x= valor a ser tatado, in_min= valor minimo de entrada, in_max= valor maximo a ser lido, out_mim= valor minimo da saida, out_max= valor maximo da saida
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void setup()
{
  Serial.begin(9600);
  // CONFIGURA OS PINOS COMO ENTRADA E SAIDA
  pinMode(REF_3V3, INPUT);                         // Pino para monitorar a tensão de referência
  pinMode(SENSOR_VELOCIDADE_VENTO_DIGITAL, INPUT); // Pino do sensor Velocidade do vento como entrada
  pinMode(SENSOR_ULTRAVIOLETA, INPUT);             // Pino do sensor Ultravioleta como entrada

  // Interrupcao 0 - pino digital 13
  // Aciona o contador a cada pulso
  attachInterrupt(SENSOR_VELOCIDADE_VENTO_DIGITAL, contador, RISING);
  pulsos = 0;
  rpm = 0;
  VELOCIDADE_VENTO = 0;
  timeold = 0;
}
void loop()
{
  // Atualiza contador a cada segundo
  if (millis() - timeold >= 1000)
  {
    detachInterrupt(SENSOR_VELOCIDADE_VENTO_DIGITAL);// Desabilita interrupcao durante o calculo
    rpm = (60 * 1000 / pulsos_por_volta) / (millis() - timeold) * pulsos; // RPM do sensor
    VELOCIDADE_VENTO = ((((2 * 3.6) * 3, 14) * 1.3) * rpm) / 60, 0;       // converte RPM em Km/h
    timeold = millis();
    pulsos = 0;
    // Mostra o valores no serial monitor
    Serial.print("Vento: ");
    Serial.print(VELOCIDADE_VENTO);
    Serial.print(" km/h");
    Serial.print("RPM = ");
    Serial.print(rpm, DEC);
    Serial.print(" | ");
    Serial.println("");
    attachInterrupt(SENSOR_VELOCIDADE_VENTO_DIGITAL, contador, RISING); // Habilita interrupcao
  }
  int uvLevel = averageAnalogRead(SENSOR_ULTRAVIOLETA, 8);
  int refLevel = averageAnalogRead(REF_3V3, 8);
  float outputVoltage = 3.3 / refLevel * uvLevel; // Use os 3,3V de alimentação no pin referência para um melhor acuidade do valor de saida do sensor
  float uvIntensity = mapfloat(outputVoltage, 0.99, 2.8, 0.0, 15.0); // Convert the voltage to a UV intensity level
}