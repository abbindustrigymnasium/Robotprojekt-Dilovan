typedef enum RobotStates {                          //Mina states
  Start, FrontWall, FRWD, TurnLeft, BackFromWall
}; 
RobotStates RState; //Döper om RobotStates för att göra det enklare

#include "EspMQTTClient.h"
#include <Servo.h> //Tar med library Servo

#define TRIG_1 12 //Framsensors trigger
#define ECHO_1 13 //Framsensors echo
#define TRIG_2 14 //Högersensors trigger
#define ECHO_2 15 //Högersensors echo

void onConnectionEstablished();
EspMQTTClient client(        //För att koppla till nätvärket med vår mikrokontroller
  "ABBIndgymIoT_2.4GHz",    // Wifi ssid
  "ValkommenHit!",         // Wifi password
  "192.168.0.112",        // MQTT broker ip
  1883,                  // MQTT broker port
  "jocke",                 // MQTT username
  "apa",                     // MQTT password
  "lyssnarlasse",           // Client name
  onConnectionEstablished, // Connection established callback
  true,                   // Enable web updater
  true                   // Enable debug messages
);
void onConnectionEstablished(){}

Servo servo; //Gör om library Servo till variabel servo
int ena = 5, Dir1 = 0;  //Som #define, säger att min motor ligger på D1 och direktion på D3
int distF, distR; //Gör variabler som sen ska användas
int Speed = 600; //Min hastighet
int servoR = 160;  //Servovärdet åt höger
int servoS = 90;  //Servovärdet fram
int servoL = 20; //Servovärdet åt vänster
int lengthFram = 12; //Distansen som framsensorn måste vara större än
int lengthRight = 10; //Distansen som högersensorn måste vara större än
unsigned long currentMillis = 0,diffMillis = 0, previousMillis = 0; //Variabler för tid

boolean timeLimit(int interval) {  //Tidsfunktion, håller tid istället för delay() som stänger allt
  currentMillis = millis(); 
  diffMillis = currentMillis - previousMillis;
  if(diffMillis > interval) {
    previousMillis = currentMillis;
    return true;
  }
  else return false;
}

void setup(){ //Setup för all hårdvara
  pinMode(ena, OUTPUT); //Till Motorn
  pinMode(Dir1, OUTPUT);  //Till Motorn
  pinMode(TRIG_1, OUTPUT);   //Till framsensorn
  pinMode(ECHO_1, INPUT);   //Till framsensorn
  pinMode(TRIG_2, OUTPUT); //Till högersensorn
  pinMode(ECHO_2, INPUT); //Till högersensorn 
  servo.attach(2); //Pin för servo = D4
  RState = Start; //Vilken state vi ska till först
} 
 
void loop (){
  client.loop(); //Connectar till nätverket
  dist_fram(); //Anropar alla distanser, fram och höger
  switch(RState){ //Går till robotstates
    
    case Start:   //Startstate, sätter allt i ordning endast en gång
      servo.write(servoS); //Vrider servot rakt 
      delay(2000);
      RState = FrontWall; //Byter state för att hitta framvägg
    break;

    case FrontWall: //State för att hitta vägg framför bilen
      client.publish("mess", "Åker fram");
      analogWrite(ena, Speed); //Åker fram
      digitalWrite(Dir1, HIGH); 
      if (distF < lengthFram){ //Tills den hittar en vägg framför sig
        previousMillis = currentMillis; //Reseta tiden
        RState = BackFromWall; //Byter state till att backa
      }
      
      if (timeLimit(8000) == true){ //Eller tills det gått 8 sekunder
        RState = BackFromWall; //Byter state till att backa
      }      
    break;

    case BackFromWall: //State för när bilen är för nära väggen
      client.publish("mess", "Backar");
      analogWrite(ena, 0); //Stanner
      delay(500);
      analogWrite(ena, Speed); //Backar i 0,4 sekunder
      digitalWrite(Dir1, LOW); 
      delay(400); 
      analogWrite(ena, 0); //Stanner igen
      servo.write(servoR); //Vrider till höger för att sedan backa ut bra
      delay(200);
      analogWrite(ena, Speed); //Backar i 0,7 s
      digitalWrite(Dir1, LOW); 
      delay(700);
      RState = TurnLeft; //Nytt state för att svänga vänster
    break;    

    case TurnLeft:  //Efter backning körs denna state/case
      client.publish("mess", "Svänger");
      servo.write(servoL); //Vrider servot åt vänster
      analogWrite(ena, Speed); //Åker rakt fram
      digitalWrite(Dir1, HIGH); 
      delay(1000); 
      client.publish("mess", "Åker fram");
      RState = FRWD; //Byter state till att åka fram
    break;
    
    case FRWD: //Case för att åka framåt och leta efter vägg på höger sida
      servo.write(servoS); //Sätter sig rakt
      analogWrite(ena, Speed); //Åker rakt fram
      digitalWrite(Dir1, HIGH);     
      if (distR < lengthRight) {  //Om den åker för nära väggen, sväng lite åt vänster
        servo.write(servoL); 
      }
      
      if (distR > lengthRight){ //Om den är för långt från väggen, sväng höger
        servo.write(servoR);  
      }
      
      if (distF < lengthFram){ //Om den åker för nära en vägg framifrån
        previousMillis = currentMillis; //Reseta tiden
        RState = BackFromWall; //Nytt state för att backa
      }
      
      if (timeLimit(12000) == true){ //Eller om den åkt för länge, den har då fastnat
        RState = BackFromWall;  //Nytt state för att backa
      }
    break;
  }
}

void dist_fram(){ //Funktion för framsensorn
  long durF; //Min duration variabel
  digitalWrite(TRIG_1, LOW); //Fixar signalerna för sonicsensorn
  delayMicroseconds(2); 
  digitalWrite(TRIG_1, HIGH);
  delayMicroseconds(10); 
  digitalWrite(TRIG_1, LOW);
  durF = pulseIn(ECHO_1, HIGH); //Tar in sista signalen från sensorn som är avståndet
  distF = (durF/2) / 29.1; //Delar på 2 och 29.1 för att få avståndet i cm
  dist_right(); //I slutet av funktionen anropas högersensor funktionen
}

void dist_right(){ //Exakt samma sak som funktionen innan för framsensorn, skillnad är att ingen funktion anropas efter slutet
  long durR;
  digitalWrite(TRIG_2, LOW);
  delayMicroseconds(2); 
  digitalWrite(TRIG_2, HIGH);
  delayMicroseconds(10); 
  digitalWrite(TRIG_2, LOW);
  durR = pulseIn(ECHO_2, HIGH);
  distR = (durR/2) / 29.1; 
}
