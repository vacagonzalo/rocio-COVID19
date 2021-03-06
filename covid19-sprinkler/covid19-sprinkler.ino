#include "config.h"

#ifdef DEBUG_MODE
  #include <Wire.h>
#endif

int estado_anterior = INICIO;
int estado_actual = INICIO;

int tiempo_rocio = TIEMPO_ROCIO;
int contador_rociadas = 0;

int encoder_tabla[16] = {0, -1000, 1000, 0, 1000, 0, 0, -1000, -1000, 0, 0, 1000
			, 0, 1000, -1000, 0};

LiquidCrystal_I2C lcd(LCD_cADDR, LCD_cCHAR, LCD_CLINE);
char segunda_linea[17] = "0123456789ABCDEF";

void setup() 
{
  #ifdef DEBUG_MODE
    Serial.begin(9600);
  #endif
  lcd.init();
  lcd.backlight();

  // sensores
  pinMode(PIN_PRESENCIA, INPUT);
  pinMode(PIN_NIVEL, INPUT);

  //encoder
  pinMode(PIN_PULSADOR, INPUT);
  pinMode(PIN_ENCODER_A, INPUT);
  pinMode(PIN_ENCODER_B, INPUT);

  // actuadores
  pinMode(PIN_ASPERSOR, OUTPUT);
  pinMode(PIN_ALARMA, OUTPUT);
}

void loop() 
{
  #ifdef DEBUG_MODE
    Serial.print("loop()\n");
  #endif
  switch (estado_actual)
  {
    case INICIO: inicio(); break;
    case REPOSO: reposo(); break;
    case ROCIAR: rociar(); break;
    case CONFIGURAR: configurar(); break;
    case ALARMAR: alarmar(); break;
  }
  delay(GUARDA_RUIDO);
}

////////////////////////////////////////////////////////////////////////////////
// Funciones de estado
////////////////////////////////////////////////////////////////////////////////

void inicio()
{
  #ifdef DEBUG_MODE
    Serial.print("inicio()\n");
  #endif
  mensaje_inicio();
  delay(TIEMPO_SALUDO);
  estado_anterior = INICIO;
  estado_actual = REPOSO;
}

void reposo()
{
  #ifdef DEBUG_MODE
    Serial.print("reposo()\n");
  #endif
  while(estado_actual == REPOSO)
  {
    if(estado_anterior != REPOSO)
    {
      #ifdef DEBUG_MODE
        Serial.print("estado_anterior != REPOSO\n");
      #endif
      mensaje_reposo();
      estado_anterior = REPOSO;
    }
    
    // Prioridad baja 
    if(digitalRead(PIN_PULSADOR) == PULSADO)
    {
      delay(GUARDA_RUIDO);
      if(digitalRead(PIN_PULSADOR) == PULSADO)
      {
        #ifdef DEBUG_MODE
          Serial.print("PIN_PULSADOR == PULSADO\n");
        #endif
        estado_actual = CONFIGURAR;
      }
    }

    // Prioriad media
    if(digitalRead(PIN_PRESENCIA) == PRESENCIA)
    {
      delay(GUARDA_RUIDO);
      if(digitalRead(PIN_PRESENCIA) == PRESENCIA)
      {
        #ifdef DEBUG_MODE
          Serial.print("PIN_PRESENCIA == PRESENCIA\n");
        #endif
        estado_actual = ROCIAR;
      }
    }

    //prioridad alta
    if(digitalRead(PIN_NIVEL) == MAL_NIVEL)
    {
      delay(GUARDA_RUIDO);
      if(digitalRead(PIN_NIVEL) == MAL_NIVEL)
      {
        #ifdef DEBUG_MODE
          Serial.print("PIN_NIVEL == MAL_NIVEL\n");
        #endif
        estado_actual = ALARMAR;
      }
    }
  }
}

void rociar()
{
  #ifdef DEBUG_MODE
      Serial.print("rociar()\n");
  #endif
  digitalWrite(PIN_ASPERSOR, ROCIANDO);
  delay(tiempo_rocio);
  digitalWrite(PIN_ASPERSOR, NO_ROCIANDO);
  estado_anterior = ROCIAR;
  estado_actual = REPOSO;
  contador_rociadas++;
  #ifdef DEBUG_MODE
      Serial.print("contador_rociadas++\n");
  #endif
  bool esta_despejado = false;
  while(!esta_despejado)
  {
    #ifdef DEBUG_MODE
      Serial.print("!esta_despejado\n");
    #endif
    delay(GUARDA_PROCESO);
    if(digitalRead(PIN_PRESENCIA) != PRESENCIA)
    {
      #ifdef DEBUG_MODE
        Serial.print("PIN_PRESENCIA != PRESENCIA\n");
      #endif
      esta_despejado = true;
    }
  }
}

void configurar()
{
  #ifdef DEBUG_MODE
    Serial.print("Se ingresa a la funcion del estado CONFIGURAR\n");
  #endif

  while(digitalRead(PIN_PULSADOR) == PULSADO); // espero a que suelte el botón
  
  bool rote_encoder = false;
  
  int a = digitalRead(PIN_ENCODER_A);
  int b = digitalRead(PIN_ENCODER_B);
  int a_viejo = a;
  int b_viejo = b;
  int indice = 10 * a + 5 * b;

  delay(GUARDA_RUIDO);
  
  while(estado_actual == CONFIGURAR)
  {
    if(estado_anterior != CONFIGURAR)
    {
      #ifdef DEBUG_MODE
        Serial.print("estado_anterior != CONFIGURAR\n");
      #endif
      mensaje_configurar();
      estado_anterior = CONFIGURAR;
    }
    if(rote_encoder)
    {
      #ifdef DEBUG_MODE
        Serial.print("rote_encoder\n");
      #endif
      mensaje_configurar();
      rote_encoder = false;
    }
    
    if(digitalRead(PIN_PULSADOR) == PULSADO)
    {
      delay(GUARDA_RUIDO);
      if(digitalRead(PIN_PULSADOR) == PULSADO)
      {
        #ifdef DEBUG_MODE
          Serial.print("PIN_PULSADOR == PULSADO\n");
        #endif
        estado_actual = REPOSO;
      }
    }
    else 
    {
      #ifdef DEBUG_MODE
        Serial.print("posicion de encoder\n");
      #endif
      a = digitalRead(PIN_ENCODER_A);
      b = digitalRead(PIN_ENCODER_B);
      if((a != a_viejo) || (b != b_viejo))
      {
        #ifdef DEBUG_MODE
          Serial.print("encoder rotado\n");
        #endif
        rote_encoder = true;
        indice = a_viejo * 8 + b_viejo * 4 + a * 2 + b;
        a_viejo = a;
        b_viejo = b;
        int t_indice = tiempo_rocio + encoder_tabla[indice];
        if(t_indice <= TIEMPO_MAXIMO && t_indice >= TIEMPO_MINIMO)
        {
          #ifdef DEBUG_MODE
            Serial.print("tiempo válido\n");
          #endif
          tiempo_rocio = t_indice; 
        }
      }
    }
  }
  while(digitalRead(PIN_PULSADOR) == PULSADO);
}

void alarmar()
{
  #ifdef DEBUG_MODE
    Serial.print("alarmar()\n");
  #endif
  while(estado_actual == ALARMAR)
  {
    #ifdef DEBUG_MODE
      Serial.print("normalidad?\n");
    #endif
    if(estado_anterior != ALARMAR)
    {
      #ifdef DEBUG_MODE
        Serial.print("estado_anterior != ALARMAR\n");
      #endif
      mensaje_alarma();
      digitalWrite(PIN_ALARMA, SUENA_ALARMA);
      estado_anterior = ALARMAR;
    }
    if(digitalRead(PIN_NIVEL) != MAL_NIVEL)
    {
      delay(GUARDA_PROCESO);
      if(digitalRead(PIN_NIVEL) != MAL_NIVEL)
      {
        #ifdef DEBUG_MODE
          Serial.print("PIN_NIVEL != MAL_NIVE\n");
        #endif
        estado_actual = REPOSO;
        digitalWrite(PIN_ALARMA, NO_SUENA_ALARMA);
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
// Funciones auxiliares a los estados
////////////////////////////////////////////////////////////////////////////////

void mensaje_inicio()
{
  lcd.setCursor(0,0);
  lcd.print(SALUDO);
  lcd.setCursor(0,1);
  lcd.print(NOMBRE_FABRICANTE);
}

void mensaje_reposo()
{
  linea_reposo();
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(NOMBRE_CLIENTE);
  lcd.setCursor(0,1);
  lcd.print(segunda_linea);
}

void mensaje_configurar()
{
  linea_configurar();
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(CONFIGURACION);
  lcd.setCursor(0,1);
  lcd.print(segunda_linea);
}

void mensaje_alarma()
{
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(ALARMA_NIVEL);
  lcd.setCursor(0,1);
  lcd.print(ALARMA_NIVEL);
}

////////////////////////////////////////////////////////////////////////////////
// Funciones auxiliares a las funciones de mensajes
////////////////////////////////////////////////////////////////////////////////

// T:15 ROCIOS:0043 <- ejemplo de línea a lograr
void linea_reposo()
{
  int rocio_segundos = tiempo_rocio / 1000;
  int rs_decenas = rocio_segundos / 10;
  int rs_unidades = rocio_segundos % 10;

  int cr_miles = contador_rociadas / 1000;
  int cr_centenas = (contador_rociadas % 1000) / 100;
  int cr_decenas = (contador_rociadas % 100) / 10;
  int cr_unidades = contador_rociadas %10;

  segunda_linea[0]  = 'T';
  segunda_linea[1]  = ':';
  segunda_linea[2]  = get_caracter(rs_decenas);
  segunda_linea[3]  = get_caracter(rs_unidades);
  segunda_linea[4]  = ' ';
  segunda_linea[5]  = 'R';
  segunda_linea[6]  = 'O';
  segunda_linea[7]  = 'C';
  segunda_linea[8]  = 'I';
  segunda_linea[9]  = 'O';
  segunda_linea[10] = 'S';
  segunda_linea[11] = ':';
  segunda_linea[12] = get_caracter(cr_miles);
  segunda_linea[13] = get_caracter(cr_centenas);
  segunda_linea[14] = get_caracter(cr_decenas);
  segunda_linea[15] = get_caracter(cr_unidades);
}

// TIEMPO DE ROC:15 <- ejemplo de línea a lograr
void linea_configurar()
{
  int rocio_segundos = tiempo_rocio / 1000;
  int rs_decenas = rocio_segundos / 10;
  int rs_unidades = rocio_segundos % 10;

  segunda_linea[0]  = 'T';
  segunda_linea[1]  = 'I';
  segunda_linea[2]  = 'E';
  segunda_linea[3]  = 'M';
  segunda_linea[4]  = 'P';
  segunda_linea[5]  = 'O';
  segunda_linea[6]  = ' ';
  segunda_linea[7]  = 'D';
  segunda_linea[8]  = 'E';
  segunda_linea[9]  = ' ';
  segunda_linea[10] = 'R';
  segunda_linea[11] = 'O';
  segunda_linea[12] = 'C';
  segunda_linea[13] = ':';
  segunda_linea[14] = get_caracter(rs_decenas);
  segunda_linea[15] = get_caracter(rs_unidades);
}

char get_caracter(int digito)
{
  char resultado;
    switch(digito)
  {
    case 0: resultado = '0'; break;
    case 1: resultado = '1'; break;
    case 2: resultado = '2'; break;
    case 3: resultado = '3'; break;
    case 4: resultado = '4'; break;
    case 5: resultado = '5'; break;
    case 6: resultado = '6'; break;
    case 7: resultado = '7'; break;
    case 8: resultado = '8'; break;
    case 9: resultado = '9'; break;
    default: resultado = '?';
  }
  return resultado;
}
