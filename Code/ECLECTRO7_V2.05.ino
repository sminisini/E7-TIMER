//--------------------------------------------------------------------------------------------------
//  CHRONO E7 V2.05
//  SMI - 20180519
//--------------------------------------------------------------------------------------------------
//  CHRONOMETRE ELECTRO 7
//
//--------------------------------------------------------------------------------------------------
String VERSION = "v2.05";


//--------------------------------------------------------------------------------------------------
//DECLARATION
//--------------------------------------------------------------------------------------------------

//LIBRAIRIES ---------------------------------------------------------------------------------------
#include <SPI.h>                      //SPI pour écran
#include <UTouch.h>                   //Tactile pour écran
#include <Adafruit_GFX.h>             //Graphique 
#include <Adafruit_ILI9341.h>         //Ecran
#include "Fonts/FreeSans9pt7b.h"      //Polices

//DECLARATION AFFICHEUR
// Uitlisation bus SPI Hardware (sur Uno/Nano, #13, #12, #11) et déclaration pour CS/DC
// #13 TFT_SCK
// #12 TFT_MISO
// #11 TFT_MOSI
#define TFT_DC 9
#define TFT_CS 10
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);
 
//DECLARATION TOUCH SCREEN
#define t_SCK  3
#define t_CS   4
#define t_MOSI 5
#define t_MISO 6
#define t_IRQ  7
UTouch ts(t_SCK, t_CS, t_MOSI, t_MISO, t_IRQ);

//CONNECTIONS --------------------------------------------------------------------------------------
#define Throttle 2           // Voie Moteur

//VARIABLES ----------------------------------------------------------------------------------------
//Gestion affichage
int XMS;                        // Position X affichage
int YMS;                        // Position Y affichage
int ETAT_TT = 0;                // Etat Affichage BP Temps travail
int ETAT_TV = 0;                // Etat Affichage BP Temps vol
int MODE = 2;                   // MODE (2:emetteur allumé/moteur arreté 1:essais moteur 0:chrono)
int memMODE = 3;                // memoire Mode pour gestion affichages
int testappui;                  // evite les appuis continu sur ecran

//Gestion chrono
unsigned long TM = 0;          // Stockage du temps Moteur
int TM_min = 0;                // Minutes de temps Moteur (calcul)
int TM_sec = 0;                // Secondes de temps Moteur (calcul)
unsigned long TV = 0;          // Stockage du temps de Vol           
int TV_min = 0;                // Minutes de temps de Vol (calcul)
int TV_sec = 0;                // Secondes de temps de Vol (calcul)
unsigned long TT = 660000;     // Stockage du temps de Travail           
int TT_min = 11;               // Minutes de temps de Travail (calcul)
int TT_sec = 0;                // Secondes de temps de Travail (calcul)
int memTT_sec;                 // memoire temps de travail
int memTV_sec;                 // memoire temps de vol
int memTM_sec;                 // memoire temps de vol

volatile byte runningTM = false; // Etat de Fonctionnement du chrono temps moteur (true = fonctionnement/false= arrêt)
volatile byte memrunningTM = false;
volatile byte runningTV = false; // Etat de Fonctionnement du chrono temps de vol (true = fonctionnement/false= arrêt)
volatile byte runningTT = false; // Etat de Fonctionnement du chrono temps de travail (true = fonctionnement/false= arrêt)
unsigned long time_now;          // Temps

//Gestion AQ
int AQ_Level;                  // niveau AQ %
int memAQ_Level;               // memoire niveau AQ pour mise à jour

//Gestion entrée voie RC (moteur)
int EtatThrottle;              // Valeur correspondant à la voie Moteur
int IniThrottle;               // Valeur initiale de la voie moteur
int OnThrottle;                // Valeur moteur en marche

//-------------------------------------------------------------------------------------------------
//INITIALISATION
//-------------------------------------------------------------------------------------------------
void setup() {
  //INITIALISATION ECRAN
  tft.begin();
  tft.setRotation(0);
  tft.fillScreen(ILI9341_BLACK); //Fond Noir
  
  //INITIALISATION TOUCH SCREEN
  ts.InitTouch(PORTRAIT);
  ts.setPrecision(PREC_MEDIUM);

  // Initialisation surveillance voie Moteur
  pinMode(Throttle, INPUT);

  //Serial.begin(9600);

}

//-------------------------------------------------------------------------------------------------
//LOOP
//-------------------------------------------------------------------------------------------------
void loop() {
  long x, y;                                                        // Déclaration des coordonnées x,y
  EtatThrottle = pulseIn(Throttle, HIGH, 30000);                    // Surveillance de la voie Moteur +/- 1.5ms
  switch (MODE) {                                                   // MODE (2:emetteur allumé/moteur arreté 1:essais moteur 0:chrono)
    case 0:   //chrono                                              // MODE 0 : Chrono
      if (memMODE != MODE) {Affichage_0 ();}                        // si changement de mode alors affichage ecran correspondant
      testappui=0;                                                  // RAZ comptage pour 1 appui sur ecran
      while(ts.dataAvailable())                                     // tant qu'il y a une entrée tactile
      {
        if (testappui == 0){                                        // test si il y a un appui sur ecran en cours (0= non; 1=oui)
          ts.read();                                                  // Lit les coordonées de l'ecran tactile
          x = ts.getX();
          y = ts.getY();
        
          //Zone 1                                                    // Gestion Zone pour Bouton 0:Départ/1:Stop/2:RAZ du temps de travail
          if(((y>=220) && (y<=280)) && ((x>=40) && (x<=100)))         // Définition de la zone tactile active
          {
            ETAT_TT = ETAT_TT + 1;                                    // si Appui, chagement état pour gestion affichage bouton
            if (ETAT_TT!=1){                                          // Gestion de l'arret du chrono temps de travail si Etat_TT different de 1 
              runningTT = false;                                
            }
            else {                                                    // Démarrage du Chrono si Etat_TT = 1 (affichage Bouton STOP)
              runningTT = true;
            }
            if(ETAT_TT>2){                                            // Si appui RAZ réinitialisation compléte
              ETAT_TT = 0;                                            // Etat_TT sur départ
              MODE = 2;                                               // Mode sur 2 pour appairage nouveau récepteur
              TT = 660000;//660000                                    // Initialisation temps de travail à 11 minutes
              TT_min=11;                                              
              TT_sec=0;
              runningTV=false;                                        // Arrêt et mise à zero chrono temps de vol
              TV=0;
              TV_min=0;
              TV_sec=0;
              TM=0;                                                   // Arrêt et mise à zero chrono temps Moteur + affichage
              TM_min=0;
              TM_sec=0;
              }
            delay(150);
            affichage_TT();                                           // Affichage des changement aspect bouton Temps de travail
          } 
        
          //zone 2
          if(((y>=0) && (y<=140)) && ((x>=0) && (x<=240)))            // Gestion Zone pour Bouton 0:Départ/1:Stop/2:RAZ du temps de vol
          {
            ETAT_TV = ETAT_TV + 1;                                    // si Appui, chagement état pour gestion affichage bouton
            if (ETAT_TV!=1){                                          // Gestion de l'arret du chrono temps de vol si Etat_TT different de 1 
              runningTV = false;
            }
            else {                                                    // Démarrage du Chrono si Etat_TT = 1 (affichage Bouton STOP)
              runningTV = true;                                         
              Affichage_chronoTV();                                   // initialisation affichage écran temps de vol au déclanchement
              
            }
            if(ETAT_TV>2){                                            // Si appui RAZ réinitialisation chrono temps de vol et temps moteur
              ETAT_TV = 0;                                            // Etat_TT sur départ
              TV=0;                                                   // Arrêt et mise à zero chrono temps de vol + affichage
              TV_min=0;
              TV_sec=0;
              Affichage_chronoTV();
              TM=0;                                                   // Arrêt et mise à zero chrono temps Moteur + affichage
              TM_min=0;
              TM_sec=0;
              Affichage_chronoTM();            
              }
            delay(150);
            affichage_TV();                                           // Affichage des changement aspect bouton Temps de travail
          }       
        }
        testappui = 1 ;                                             // Appuis en cours
      }
      chrono();                                                     // Appels void gestion des chronos    
      break;  // End Mode 0                       
    case 1:   //essais moteur                                       // MODE 1 : Essai Moteur
      if (memMODE != MODE) {Affichage_1 ();}                        // si changement de mode alors affichage ecran correspondant
      if (EtatThrottle < 0.95*IniThrottle || EtatThrottle > 1.05*IniThrottle){    // Si la Valeur de la voie moteur varie de 5% alors détection du sens d'action
        OnThrottle = EtatThrottle;                                                // Stockage de la valeur Moteur en Marche (Sens)
        runningTM = true;
      }
      else  {
        //OnThrottle = EtatThrottle;
        runningTM = false;    
        }
      if (memrunningTM  !=runningTM){
      Afficage_EtatThrottle();
      }
      memrunningTM = runningTM ; 
      while(ts.dataAvailable())                                     // tant qu'il y a une entrée tactile
      {
        ts.read();                                                  // Lit les coordonées de l'ecran tactile
        x = ts.getX();
        y = ts.getY();
        
        //zone 2                                                    // Gestion Zone pour Bouton Validation
        if(((y>=0) && (y<=140)) && ((x>=0) && (x<=240)))            // Définition de la zone tactile active
        {

          MODE=0;                                                   // Changement mode sur validation
        }       
      }    
      break; // End Mode 1
    case 2:   //emetteur allumé/moteur arreté                       // MODE 2 : emetteur allumé/moteur arreté
      if (memMODE != MODE) {Affichage_2 ();}                        // si changement de mode alors affichage ecran correspondant  
      while(ts.dataAvailable())                                     // tant qu'il y a une entrée tactile
      {
        ts.read();                                                  // Lit les coordonées de l'ecran tactile
        x = ts.getX();
        y = ts.getY();
        
        //zone 2                                                    // Gestion Zone pour Bouton Validation
        if(((y>=0) && (y<=140)) && ((x>=0) && (x<=240)))            // Définition de la zone tactile active
        { 
          IniThrottle = EtatThrottle;                               // Stockage de la valeur Moteur Coupé
          MODE=1;                                                   // Changement mode sur validation
        }       
      }
      break;  // End Mode 2
  }//End Switch Mode

  AQ_Level = 100 ; //random(0,100);                                  // Gestion de la tension de l'AQ
  if (AQ_Level!=memAQ_Level){                                       // mise a jour affichage uniquement sur changment de valeur 
  tft_NIVEAUAQ();
  }
  memAQ_Level = AQ_Level;                                           // mise en mémoire de la derniere valeur de l'AQ
  
}//End void Loop

//-------------------------------------------------------------------------------------------------
//CHRONO
//-------------------------------------------------------------------------------------------------
void chrono() {
  static unsigned long last_time = 0;             // Temps antérieur
  time_now = millis();                            // Temps actuel
   
                                                  // CHRONO TEMPS DE TRAVAIL
  if (runningTT && TT>0){                         // si le chrono est en marche               
    TT = TT - (time_now - last_time);             // Calcul du temps de travail
    TT_min = int((TT/1000)/60);                   // Calcul des minutes de travail pour affichage
    TT_sec = TT/1000 - int((TT/1000)/60)*60;      // Calcul des secondes de travail pour affichage
    if (TT_sec!=memTT_sec){                       // mise à jour de l'affichage si changement
      Affichage_chronoTT();
    }
  }// Fin Comptage temps travail 

                                                  // CHRONO TEMPS DE VOL
  if (runningTV){                                 // si le chrono est en marche               
    TV = TV + (time_now - last_time);             // Calcul du temps de travail
    TV_min = int((TV/1000)/60);                   // Calcul des minutes de travail pour affichage
    TV_sec = TV/1000 - int((TV/1000)/60)*60;      // Calcul des secondes de travail pour affichage
    if (TV_sec!=memTV_sec){                       // mise à jour de l'affichage si changement
      Affichage_chronoTV();
    }

    if (((OnThrottle-IniThrottle>0) && (EtatThrottle > 1.05 * IniThrottle)) || ((OnThrottle-IniThrottle<0) && (EtatThrottle < 0.95 * IniThrottle))){    // et si Moteur en marche
        TM = TM + (time_now - last_time);         // Calcul du temps Moteur
        TM_min = int((TM/1000)/60);               // Calcul des minutes Moteur pour affichage
        TM_sec = TM/1000 - int((TM/1000)/60)*60;  // Calcul des secondes Moteur pour affichage
        if (TM_sec!=memTM_sec){                       // mise à jour de l'affichage si changement
          Affichage_chronoTM();
        }        
    }// Fin Comptage temps Moteur   
  }// Fin Comptage temps travail
    
  last_time = time_now;                           // memoire du temps actuelle
  memTT_sec=TT_sec;                               // memoire des secondes du temps de travail
  memTV_sec=TV_sec;                               // memoire des secondes du temps de vol
  memTM_sec=TM_sec;                               // memoire des secondes du temps Moteur  
}//end void chrono

//-------------------------------------------------------------------------------------------------
//GESTION AFFICHAGE PARTIELLE (Modification de valeurs)
//-------------------------------------------------------------------------------------------------

//PAGES 1,2,3 - NIVEAU AQ ----------------------------------------------------------------------------------
void tft_NIVEAUAQ(){
  XMS = 0; YMS = 0;  
  tft.fillRect(XMS+160,YMS,240,20,ILI9341_WHITE);
  
  if (AQ_Level<=15){
    tft.setTextColor(ILI9341_RED);
  }
  else {
    tft.setTextColor(ILI9341_BLACK);
  }
  tft.setFont(&FreeSans9pt7b);
  tft.setTextSize(1); 
  if(AQ_Level<10){tft.setCursor(XMS+161+20,YMS+15);}
  if(AQ_Level>=10){tft.setCursor(XMS+161+10,YMS+15);}
  if(AQ_Level>=100){tft.setCursor(XMS+161,YMS+15);}  
  tft.print(AQ_Level);
  tft.setCursor(XMS+161+30,YMS+15);
  tft.print("%");

  if (AQ_Level<=15){
    tft.drawRect(XMS+214,YMS+3,24,14,ILI9341_RED);
    tft.fillRect(XMS+210,YMS+5,4,10,ILI9341_RED);
    tft.fillRect(XMS+216+(20-AQ_Level/5),YMS+5,AQ_Level/5,10,ILI9341_RED);
  }
  else{
    tft.drawRect(XMS+214,YMS+3,24,14,ILI9341_BLACK);
    tft.fillRect(XMS+210,YMS+5,4,10,ILI9341_BLACK);
    tft.fillRect(XMS+216+(20-AQ_Level/5),YMS+5,AQ_Level/5,10,ILI9341_BLACK);
  }
}//Fin tft_NIVEAUAQ

//Affichage bouton Temps de travail ----------------------------------------------------------------
void affichage_TT(){
  YMS = 30;
  XMS = 4;
        //tft.fillRect(XMS+140,YMS,94,60,ILI9341_BLACK);
  switch (ETAT_TT) {
    case 0:   //DEPART
        tft.fillRoundRect(XMS+140,YMS+5,91,61,7,ILI9341_GREEN);
        tft.setTextColor(ILI9341_WHITE);
        tft.setFont(&FreeSans9pt7b);
        tft.setTextSize(1);  
        tft.setCursor(XMS+136,YMS+40);
        tft.print("   DEPART");
        break;        
    case 1:   //STOP
        tft.fillRoundRect(XMS+140,YMS+5,91,61,7,ILI9341_RED);
        tft.setTextColor(ILI9341_WHITE);
        tft.setFont(&FreeSans9pt7b);
        tft.setTextSize(1);  
        tft.setCursor(XMS+136,YMS+40);
        tft.print("     STOP");
        break; 
    case 2:   //RAZ
        tft.fillRoundRect(XMS+140,YMS+5,91,61,7,ILI9341_YELLOW);
        tft.setTextColor(ILI9341_BLACK);
        tft.setFont(&FreeSans9pt7b);
        tft.setTextSize(1);  
        tft.setCursor(XMS+136,YMS+30);
        tft.print("   REMISE");
        tft.setCursor(XMS+136,YMS+50);
        tft.print("   A ZERO");
        break;
    }//FIN SWITCH
}

//Affichage bouton Temps de vol ----------------------------------------------------------------
void affichage_TV(){    
  XMS = 4; YMS = 113;
  switch (ETAT_TV) {
    case 0:   //DEPART
      tft.fillRoundRect(XMS+2,YMS+82,228,118,7,ILI9341_GREEN);
      tft.setTextColor(ILI9341_WHITE);
      tft.setFont(&FreeSans9pt7b);
      tft.setTextSize(2);  
      tft.setCursor(XMS+2,YMS+150);
      tft.print("    DEPART");
    break;
    case 1:   //STOP
      tft.fillRoundRect(XMS+2,YMS+82,228,118,7,ILI9341_RED);
      tft.setTextColor(ILI9341_WHITE);
      tft.setFont(&FreeSans9pt7b);
      tft.setTextSize(2);  
      tft.setCursor(XMS+2,YMS+150);
      tft.print("       STOP");   
    break;
    case 2:   //RAZ    
      tft.fillRoundRect(XMS+2,YMS+82,228,118,7,ILI9341_YELLOW);
      tft.setTextColor(ILI9341_BLACK);
      tft.setFont(&FreeSans9pt7b);
      tft.setTextSize(2);  
      tft.setCursor(XMS+2,YMS+130);
      tft.print("     REMISE");
      tft.setCursor(XMS+2,YMS+170);
      tft.print("     A ZERO");   
    break;
    }//FIN SWITCH  
}

//Affichage chrono Temps de travail ----------------------------------------------------------------
void Affichage_chronoTT(){  
  //Temps de travail
  XMS = 4; YMS = 30;   
  tft.fillRoundRect(XMS,YMS+20,135,50,5,ILI9341_BLACK);
  tft.setTextColor(ILI9341_WHITE);
  tft.setFont(&FreeSans9pt7b);
  tft.setTextSize(3);
    
  tft.setCursor(XMS,YMS+61);
  if(TT_min<10){
    tft.print("0");
    tft.setCursor(XMS+30,YMS+61);
    tft.print(TT_min);
    }
  else {tft.print(TT_min);}
  
  tft.setCursor(XMS+60,YMS+61);
  tft.print(":");
  
  tft.setCursor(XMS+75,YMS+61);
  if(TT_sec<10){
    tft.print("0");
    tft.setCursor(XMS+105,YMS+61);
    tft.print(TT_sec);
    }
  else {tft.print(TT_sec);}  
}//end void Affichage_chronoTT

//Affichage chrono Temps de vol ----------------------------------------------------------------
void Affichage_chronoTV(){    
  //Temps de vol
  XMS = 4; YMS = 113;  
  tft.fillRoundRect(XMS,YMS+20,135,50,5,ILI9341_BLACK);
  if(runningTV==true && TV<600000) {
   tft.setTextColor(ILI9341_GREEN); 
  }
  else {tft.setTextColor(ILI9341_WHITE);} 
  if(TV>600000){
    tft.setTextColor(ILI9341_RED);
  } 
  tft.setFont(&FreeSans9pt7b);
  tft.setTextSize(3);
    
  tft.setCursor(XMS,YMS+61);
  if(TV_min<10){
    tft.print("0");
    tft.setCursor(XMS+30,YMS+61);
    tft.print(TV_min);
    }
  else {tft.print(TV_min);}
  
  tft.setCursor(XMS+60,YMS+61);
  tft.print(":");
  
  tft.setCursor(XMS+75,YMS+61);
  if(TV_sec<10){
    tft.print("0");
    tft.setCursor(XMS+105,YMS+61);
    tft.print(TV_sec);
    }
  else {tft.print(TV_sec);}  
}//end void Affichage_chronoTV


//Affichage chrono Temps de vol ----------------------------------------------------------------
void Affichage_chronoTM(){
  XMS = 4; YMS = 113; 

  tft.fillRect(XMS+140,YMS+28,95,30,ILI9341_BLACK);
  tft.setTextColor(ILI9341_WHITE);
  
  tft.setFont(&FreeSans9pt7b);
  tft.setTextSize(2);  
  tft.setCursor(XMS+140,YMS+55);
  if(TM_min<10){
    tft.print("0");
    tft.setCursor(XMS+20+140,YMS+55);
    tft.print(TM_min);
    }
  else {tft.print(TM_min);}
  
  tft.setCursor(XMS+40+140,YMS+55);
  tft.print(":");
  
  tft.setCursor(XMS+50+140,YMS+55);
  if(TM_sec<10){
    tft.print("0");
    tft.setCursor(XMS+70+140,YMS+55);
    tft.print(TM_sec);
    }
  else {tft.print(TM_sec);}   
}//end void Affichage_chronoTM

//Affichage Etat Moteur à l'éssais ----------------------------------------------------------------
void Afficage_EtatThrottle(){
  YMS = 100;
  XMS = 60;
  
  if (EtatThrottle < 0.95*IniThrottle || EtatThrottle > 1.05*IniThrottle){    // Si la Valeur de la voie moteur varie de 5% alors détection du sens d'action
      tft.fillRoundRect(XMS+10,YMS+5,100,25,5,ILI9341_BLACK);
      tft.fillRoundRect(XMS+10,YMS+5,100,25,5,ILI9341_RED);
      tft.setTextColor(ILI9341_WHITE);
      tft.setTextSize(1);
      tft.setCursor(XMS+17,YMS+23);
      tft.print("En Marche");
  } 
  else {
      tft.fillRoundRect(XMS+10,YMS+5,100,25,5,ILI9341_BLACK);
      tft.drawRoundRect(XMS+10,YMS+5,100,25,5,ILI9341_WHITE);
      tft.setTextColor(ILI9341_WHITE);
      tft.setTextSize(1);
      tft.setCursor(XMS+28,YMS+23);
      tft.print("A l'arret");
  }
}


//-------------------------------------------------------------------------------------------------
//GESTION DES ECRANS
//-------------------------------------------------------------------------------------------------

//ECRAN 2 - Emeteur alummé/Moteur arreté ----------------------------------------------------------
void Affichage_2 (){    //emetteur allumé/moteur arreté
  memMODE = MODE;
  tft.fillScreen(ILI9341_BLACK); //Fond Noir
    
  //GENERAL
  YMS = 0;
  XMS = 0;

  //Titre Chrono + Charge AQ
  XMS = 0; YMS = 0;
  tft.fillRect(XMS,YMS,160,20,ILI9341_WHITE);
  tft.setTextColor(ILI9341_BLACK);
  tft.setFont(&FreeSans9pt7b);
  tft.setTextSize(1);  
  tft.setCursor(XMS+4,YMS+15);
  tft.print("Chrono E7");
  tft.setCursor(XMS+4+90,YMS+15);
  tft.print(VERSION);
  
  tft_NIVEAUAQ();

  //Etape
  XMS = 120; YMS = 50;  
  tft.drawCircle(XMS,YMS,20,ILI9341_WHITE);
  tft.setTextColor(ILI9341_WHITE);
  tft.setFont(&FreeSans9pt7b);
  tft.setTextSize(2);   
  tft.setCursor(XMS-10,YMS+10);
  tft.print("1");

  //Message
  XMS = 50; YMS = 100;  
  tft.setTextColor(ILI9341_WHITE);
  tft.setFont(&FreeSans9pt7b);
  tft.setTextSize(1);   
  tft.setCursor(XMS+7,YMS);
  tft.print("Emetteur allume");
  tft.setCursor(XMS+10,YMS+20);
  tft.print("Moteur arrete ?");
  
  //BP VALIDER
  XMS = 4; YMS = 113;
  tft.fillRoundRect(XMS+2,YMS+82,228,118,7,ILI9341_GREEN);
  tft.setTextColor(ILI9341_WHITE);
  tft.setFont(&FreeSans9pt7b);
  tft.setTextSize(2);  
  tft.setCursor(XMS+2,YMS+150);
  tft.print("         OK");
  
}

//ECRAN 1 - Essais moteur ---------------------------------------------
void Affichage_1 (){    //essai moteur
  memMODE = MODE;
  tft.fillScreen(ILI9341_BLACK); //Fond Noir
  
  //GENERAL
  YMS = 0;
  XMS = 0;

  //Titre Chrono + Charge AQ
  XMS = 0; YMS = 0;
  tft.fillRect(XMS,YMS,240,20,ILI9341_WHITE);
  tft.setTextColor(ILI9341_BLACK);
  tft.setFont(&FreeSans9pt7b);
  tft.setTextSize(1);  
  tft.setCursor(XMS+4,YMS+15);
  tft.print("Chrono E7");
  tft.setCursor(XMS+4+90,YMS+15);
  tft.print(VERSION);

  tft_NIVEAUAQ();

  //Etape
  XMS = 120; YMS = 50;  
  tft.drawCircle(XMS,YMS,20,ILI9341_WHITE);
  tft.setTextColor(ILI9341_WHITE);
  tft.setFont(&FreeSans9pt7b);
  tft.setTextSize(2);   
  tft.setCursor(XMS-10,YMS+10);
  tft.print("2");  

  //Message
  XMS = 60; YMS = 100;  
  tft.setTextColor(ILI9341_WHITE);
  tft.setFont(&FreeSans9pt7b);
  tft.setTextSize(1);   
  tft.setCursor(XMS,YMS);
  tft.print("Essais moteur :");

//  tft.fillRoundRect(XMS+10,YMS+5,100,25,5,ILI9341_RED);
//  tft.setTextColor(ILI9341_WHITE);
//  tft.setCursor(XMS+17,YMS+23);
//  tft.print("En Marche");

  tft.drawRoundRect(XMS+10,YMS+5,100,25,5,ILI9341_WHITE);
  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(XMS+28,YMS+23);
  tft.print("A l'arret");
  
   
  //BP VALIDER
  XMS = 4; YMS = 113;
  tft.fillRoundRect(XMS+2,YMS+82,228,118,7,ILI9341_GREEN);
  tft.setTextColor(ILI9341_WHITE);
  tft.setFont(&FreeSans9pt7b);
  tft.setTextSize(2);  
  tft.setCursor(XMS+2,YMS+150);
  tft.print("   ESSAI OK"); 
  
}

//ECRAN 0 - Chrono ---------------------------------------------
void Affichage_0(){
  memMODE = MODE;
  tft.fillScreen(ILI9341_BLACK); //Fond Noir
  
  //GENERAL
  YMS = 0;
  XMS = 0;

  //Titre Chrono + Charge AQ
  XMS = 0; YMS = 0;
  tft.fillRect(XMS,YMS,240,20,ILI9341_WHITE);
  tft.setTextColor(ILI9341_BLACK);
  tft.setFont(&FreeSans9pt7b);
  tft.setTextSize(1);  
  tft.setCursor(XMS+4,YMS+15);
  tft.print("Chrono E7");
  tft.setCursor(XMS+4+90,YMS+15);
  tft.print(VERSION);

  tft_NIVEAUAQ();
  
  //Temps de travail
  XMS = 4; YMS = 30;      
  tft.drawRoundRect(XMS-4,YMS,240,71,10,ILI9341_WHITE);
  tft.fillRoundRect(XMS-4,YMS,140,20,10,ILI9341_WHITE);
  tft.fillRect(XMS-4,YMS+10,130,20,ILI9341_WHITE);
  tft.fillRect(XMS+6,YMS,180,10,ILI9341_WHITE);
  tft.fillRoundRect(XMS-3,YMS+20,238,50,10,ILI9341_BLACK);
  tft.fillRoundRect(XMS+136,YMS+1,99,69,10,ILI9341_BLACK);
  
  tft.setTextColor(ILI9341_BLACK);
  tft.setFont(&FreeSans9pt7b);
  tft.setTextSize(1);  
  tft.setCursor(XMS+5,YMS+15);
  tft.print("Tps de Travail");
  
  tft.setTextColor(ILI9341_WHITE);
  tft.setFont(&FreeSans9pt7b);
  tft.setTextSize(3);
  
  tft.setCursor(XMS,YMS+61);
  if(TT_min<10){
    tft.print("0");
    tft.setCursor(XMS+30,YMS+61);
    tft.print(TT_min);
    }
  else {tft.print(TT_min);}
  
  tft.setCursor(XMS+60,YMS+61);
  tft.print(":");
  
  tft.setCursor(XMS+75,YMS+61);
  if(TT_sec<10){
    tft.print("0");
    tft.setCursor(XMS+105,YMS+61);
    tft.print(TT_sec);
    }
  else {tft.print(TT_sec);}

  //BP Tps Travail
  tft.fillRoundRect(XMS+140,YMS+5,91,61,7,ILI9341_GREEN);
  tft.setTextColor(ILI9341_BLACK);
  tft.setFont(&FreeSans9pt7b);
  tft.setTextSize(1);  
  tft.setCursor(XMS+136,YMS+40);
  tft.print("   DEPART");
  
  //Temps de vol
  XMS = 4; YMS = 113; 
  //tft.fillRoundRect(XMS-4,YMS,240,207,10,ILI9341_WHITE);
  tft.drawRoundRect(XMS-4,YMS,240,207,10,ILI9341_WHITE);
  tft.fillRoundRect(XMS-4,YMS,140,20,10,ILI9341_WHITE);
  tft.fillRect(XMS-4,YMS+10,130,20,ILI9341_WHITE);
  tft.fillRect(XMS+6,YMS,180,10,ILI9341_WHITE);
  tft.fillRoundRect(XMS-3,YMS+20,238,205-19,10,ILI9341_BLACK);
  tft.fillRoundRect(XMS+136,YMS,100,70,10,ILI9341_BLACK);
  tft.drawRoundRect(XMS+136,YMS,100,67,10,ILI9341_WHITE); 
  
  tft.setTextColor(ILI9341_BLACK);
  tft.setFont(&FreeSans9pt7b);
  tft.setTextSize(1);  
  tft.setCursor(XMS+5,YMS+15);
  tft.print("Temps de Vol");

  tft.setTextColor(ILI9341_WHITE);
  tft.setFont(&FreeSans9pt7b);
  tft.setTextSize(3);
  
  tft.setCursor(XMS,YMS+61);
  if(TV_min<10){
    tft.print("0");
    tft.setCursor(XMS+30,YMS+61);
    tft.print(TV_min);
    }
  else {tft.print(TV_min);}
  
  tft.setCursor(XMS+60,YMS+61);
  tft.print(":");
  
  tft.setCursor(XMS+75,YMS+61);
  if(TV_sec<10){
    tft.print("0");
    tft.setCursor(XMS+105,YMS+61);
    tft.print(TV_sec);
    }
  else {tft.print(TV_sec);}

  //Temps moteur
  tft.setTextColor(ILI9341_WHITE);
  tft.setFont(&FreeSans9pt7b);
  tft.setTextSize(1);
  tft.setCursor(XMS+140,YMS+22);
  tft.print("Tps Moteur");
  tft.setTextColor(ILI9341_WHITE);
  tft.setFont(&FreeSans9pt7b);
  tft.setTextSize(2);  
  tft.setCursor(XMS+140,YMS+55);
  //tft.print("00:00");

  tft.setCursor(XMS+140,YMS+55);
  if(TM_min<10){
    tft.print("0");
    tft.setCursor(XMS+20+140,YMS+55);
    tft.print(TM_min);
    }
  else {tft.print(TM_min);}
  
  tft.setCursor(XMS+40+140,YMS+55);
  tft.print(":");
  
  tft.setCursor(XMS+50+140,YMS+55);
  if(TV_sec<10){
    tft.print("0");
    tft.setCursor(XMS+70+140,YMS+55);
    tft.print(TM_sec);
    }
  else {tft.print(TM_sec);} 


  //BP Tps Vol
  tft.fillRoundRect(XMS+2,YMS+82,228,118,7,ILI9341_GREEN);
  tft.setTextColor(ILI9341_BLACK);
  tft.setFont(&FreeSans9pt7b);
  tft.setTextSize(2);  
  tft.setCursor(XMS+2,YMS+150);
  tft.print("    DEPART");
}



