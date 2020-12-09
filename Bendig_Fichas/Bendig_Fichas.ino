#include <SPI.h>
#include <SD.h> //Librerias SD
#include <LiquidCrystal_I2C.h>

//Crear el objeto lcd  dirección  0x3F y 16 columnas x 2 filas
LiquidCrystal_I2C lcd(0x27, 20, 4);

//Constantes para los pines.
const int SignalPin = 2;
const int SignalBtn = 3;
const int UnPeso = 1;
volatile int pulso = 0;
volatile unsigned long MillisUltPulso = 0;
int PulsosAcum = 0;
int CreditoAcum = 0;
int MaxTimePulse = 70;
int CS = 4;

//Variables
File archivoPIN;
File archivoIDX;
String index = "";
String pin = "";
int EstadoBtn = 0;

String validez = "";
String indexArchivo = "";
String pinArchivo = "";


void setup()
{
  lcd.init();// Inicializar el LCD
  lcd.backlight();
  pinMode(SignalBtn, INPUT);
  pinMode(SignalPin, INPUT_PULLUP);
  // Interrupción 0 (INT0) por el pin digital 2 que activa la ISR en cada flanco de subida
  attachInterrupt(digitalPinToInterrupt(SignalPin), coinInterrupt, RISING);
  if (!SD.begin(CS))
  {
    return;
  }
  Start();
}
void Start() {
  lcd.setCursor(2, 1);
  lcd.print("Inserte monedas");
  lcd.setCursor(4, 2);
  lcd.print("Creditos: 0");
}

void loop()
{
  //Calculamos los milisegundos de la ultima ejecusion menos el ultimo tiempo que se genero un pulso.
  unsigned long lastTime = millis() - MillisUltPulso;

  //Validamos si hay algun puslo, si es asi tambien se valida que el ultimo tiempo asignado sea mayor a la cantidad de milisegundos establecidos.
  if ((pulso > 0) && (lastTime > MaxTimePulse)) {
    //La cantidad de creditos es el contador y acumulador de pulsos, hasta que se cumpla alguna condicion.
    PulsosAcum += pulso;
    pulso = 0;
    if (PulsosAcum == UnPeso) {
      lcd.clear();
      CreditoAcum += 1;
      lcd.setCursor(4, 1);
      lcd.print("Credito Total:   ");
      lcd.setCursor(7, 2);
      lcd.print(String("$") + CreditoAcum + String(".00"));
      PulsosAcum = 0;
    }
    if (EstadoBtn == 1 || EstadoBtn == 3) {
      lcd.clear();
      Start();
      EstadoBtn = 0;
      resetVariables();
      CreditoAcum += 1;
    }
  }
  if (digitalRead(SignalBtn) == HIGH && EstadoBtn == 0 ) {
    if (CreditoAcum == 5) {  // 30 minutos
      indexArchivo = "PIN30m/INDEX30m.txt";
      pinArchivo = "PIN30m/PIN30m.txt";
      validez = "30 min";
      mostrarFicha();

    } else if (CreditoAcum == 10) {  // 1 hora
      indexArchivo = "PIN1H/INDEX1H.txt";
      pinArchivo = "PIN1H/PIN1H.txt";
      validez = "1 hrs";
      mostrarFicha();

    } else if (CreditoAcum == 20) { // 1 dia
      indexArchivo = "PIN1D/INDEX1D.txt";
      pinArchivo = "PIN1D/PIN1D.txt";
      validez = "1 dia";
      mostrarFicha();

    } else if (CreditoAcum == 60) { // 1 semana
      indexArchivo = "PIN7D/INDEX7D.txt";
      pinArchivo = "PIN7D/PIN7D.txt";
      validez = "7 dias";
      mostrarFicha();

    } else if (CreditoAcum == 80) { // 15 dias
      indexArchivo = "PIN15D/INDEX15D.txt";
      pinArchivo = "PIN15D/PIN15D.txt";
      validez = "15 dias";
      mostrarFicha();

    } else if (CreditoAcum == 160) { // 1 mes
      indexArchivo = "PIN30D/INDEX30D.txt";
      pinArchivo = "PIN30D/PIN30D.txt";
      validez = "1 mes";
      mostrarFicha();
    }
    delay(1000);
  } else if (digitalRead(SignalBtn) == HIGH && EstadoBtn == 1) {
    lcd.clear();
    Start();
    EstadoBtn = 0;
    resetVariables();
    delay(1000);
  } else if (digitalRead(SignalBtn) == HIGH && EstadoBtn == 3) {
    lcd.clear();
    Start();
    resetVariables();
    EstadoBtn = 0;
    delay(1000);
  }
}

// Interrupcion.
void coinInterrupt() {
  // Cada vez que insertamos una moneda valida, incrementamos el contador de monedas y encendemos la variable de control,
  pulso++;
  MillisUltPulso = millis();
}
void cambiarIndex() {
  archivoIDX = SD.open(indexArchivo, FILE_WRITE | O_TRUNC);
  if (archivoIDX) {
    // Serial.println("Nuevo index " + index);
    archivoIDX.print(index);
    archivoIDX.close();
    index = "";
  } else {
  }
}
void mostrarFicha() {
  //Leer Index
  File archivoIDX = SD.open(indexArchivo, FILE_READ);
  if (archivoIDX) {
    while (archivoIDX.available()) {
      char caracterI = archivoIDX.read();
      index = index + caracterI;
    }
    archivoIDX.close();
    Serial.println("Index: " + index);
  }
  // Leer PIN
  int pos = String(index).toInt();
  archivoPIN = SD.open(pinArchivo, FILE_READ);
  archivoPIN.seek(pos);
  Serial.println(pos);
  if (archivoPIN) {
    while (archivoPIN.available()) {
      char carPIN = archivoPIN.read();
      pin = pin + carPIN;
      if (carPIN == '\n') {
        break;
      }
    }
    archivoPIN.close();
    lcd.clear();
    lcd.setCursor(5, 1);
    lcd.print("Tu PIN es:");
    String pin2 = pin;
    pin2.trim();
    lcd.setCursor(3, 2);
    lcd.print(">>> "+String(pin2)+" <<<");
    lcd.setCursor(1, 3);
    lcd.print(String("Valido por: ") + validez );

    Serial.println(pin.length());
    Serial.println("PIN: " + pin);
    pos += pin.length();

    index = pos;
    cambiarIndex();
    EstadoBtn = 1;

  } else {
    errorPin();
  }
}
void errorPin() {
  // *** Error al abrir archivo pin ***
  lcd.clear();
  lcd.setCursor(2, 0);
  lcd.print("**** ERROR P ****");
  lcd.setCursor(1, 1);
  lcd.print("Pide tu reembloso");
  lcd.setCursor(4, 2);
  lcd.print(String("por: $") + CreditoAcum + String(".00"));
  lcd.setCursor(1, 3);
  lcd.print(pinArchivo);
  EstadoBtn = 3;
}
void resetVariables () {
  // resetear variables
  PulsosAcum = 0;
  CreditoAcum = 0;
  index = "";
  pin = "";
  validez = "";
}
