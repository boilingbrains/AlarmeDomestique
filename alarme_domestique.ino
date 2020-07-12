//Importation des bibliothèques nécessaires au projet
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <Password.h>

LiquidCrystal_I2C lcd(0x27, 16, 2); //L'adresse de l'lcd pourrait etre différente de 0x27 -> si besoin lancez un code pour scanner les périphériques I2C (I2C Scanner)
//On spécifie les pins sur lesquelles sont connectés le détecteur, la led et le buzzer
int detecteurMouvement = 8;
int led = 9;
int buzzer = 10;

const byte lignes = 4; //Nombre de lignes et colonnes sur le clavier
const byte colonnes = 4;
char boutons[lignes][colonnes] = { //Matrice de caractères correspondant à chacun des boutons du clavier
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte pinsLignes[lignes] = {7, 6, 5, 4}; //On spécifie les pins où sont branchés les fils correspondant aux lignes du clavier
byte pinsColonnes[colonnes] = {3, 2, 1, 0}; //On spécifie les pins où sont branchés les fils correspondant aux colonnes du clavier
Keypad clavier = Keypad(makeKeymap(boutons), pinsLignes, pinsColonnes, lignes, colonnes);

char bouton; //Variable permettant de stocker le caractère du bouton sur lequel on a appuyé
boolean alarme = false; //Variable permettant de savoir si l'alarme est activée ou désactivée
boolean code = false; //Variable permettant de controler si le code entré est correct ou non
boolean detection = false; //Variable permettant de savoir si un mouvement à été détecté
boolean timeout = false; //Variable permettant de savoir si on a dépassé le temps dans l'état ALERTE et qu'on doit passer en MODE SIRENE
boolean tempsMort = true; //Variable qui permet de gérer l'inactivité du détecteur de mouvement
int etape = 1; //Variable permettant de contrôler les événements dans le loop()
unsigned long tempoSirene = 0; //Variable compteur permettant de savoir quand activer le buzzer dans le MODE SIRENE tout en évitant les delay() qui sont bloquant !!
int compteur = 0; //Variable compteur permettant de compter le nombre de bip dans le MODE ALERTE
long temps; //Variable utilisée pour des mesures de temps locales lorsque l'alarme est en MODE ALERTE
int posLCD = 12; //Position sur l'LCD (colonne) où on print le 1er caractère du code
Password motDePasse = Password("1234"); //Mot de passe permettant d'activer et désactiver l'alarme


void setup() {
  //Déclaration des pins de l'Arduino en sortie pour la led et le buzzer et en entrée pour le détecteur
  pinMode(led, OUTPUT);
  pinMode(buzzer, OUTPUT);
  pinMode(detecteurMouvement, INPUT);
  //Initialisation de l'LCD et activation du rétro-éclairage
  lcd.init();
  lcd.backlight();
  //Appel de la fonction (voir plus bas dans le code) qui gère l'affichage de l'état de l'alarme sur l'LCD -> par défaut l'alarme est désactivée
  ecranEtatAlarme();
}


void loop() {
  bouton = clavier.getKey(); //On lit le clavier et on stocke le résultat dans la variable bouton
  if (bouton != NO_KEY) {
    switch (bouton)//On exécute une action en fonction de la touche qui a été appuyée
    {
      case '*' : reinitialiserCode(); break; //Si on appuye sur *, on efface le code sur l'LCD
      case '#' : verifierCode(); break; //Si on appuye sur #, on vérifie que le code entré par l'utilisateur = mot de passe de l'alarme
      case 'A' : break; //On ne fait rien lorsqu'il s'agit des 4 lettres
      case 'B' : break;
      case 'C' : break;
      case 'D' : break;
      default : ajouter(); //Si on appuye sur un chiffre, on l'ajoute au code utilisateur (concaténation) et on print sur l'LCD
    }
  }

  //ETAT 1
  //Si l'alarme vient d'être ACTIVEE,on attend 15 secondes et puis on commence à prendre des mesures avec le détecteur de mouvement
  //Dès que le capteur a détecté un mouvement, l'alarme passe en MODE ALERTE 
  if (alarme == true && etape == 1) {
    if (tempsMort) {
      delay(15000);
      tempsMort = false;
    }
    else {
      if (digitalRead(detecteurMouvement) == HIGH) {
        detection = true;
        etape = 2;
      }
    }
  }

  //ETAT 2 (MODE ALERTE)
  //On émet un bip toutes les secondes pendant environ 10 secondes (=tant que compteur <=10)
  //On utilise la fonction millis pour la mesure du temps à la place d'un delay (bloquant) qui rendrait le clavier indisponible or on veut lire le clavier à tout moment
  if (detection == true && etape == 2) { 
    if ((millis() - temps) > 1000)
    {
      tone(buzzer, 1000, 50);
      temps = millis();
      compteur = compteur + 1;

      if (compteur > 10) {
        timeout = true;
        etape = 3;
        compteur = 0;
      }
    }
  }

  //ETAT 3 (MODE SIRENE)
  //On active le buzzer pendant 10000 tours de boucle et on le désactive pendant 10000 autres tours
  //Le choix du nombre de boucle s'est fait par essai-erreur jusqu'à obtenir le résultat sonore souhaité
  //Une fois de plus, on évite la fonction delay grâce à une nouvelle astuce
  if (timeout == true) { //L'alarme passe en MODE SIRENE
    if (tempoSirene < 10000) {
      tone(buzzer, 1000, 50);
    }
    else {
      noTone(buzzer);
    }
    tempoSirene = tempoSirene + 1;
    if (tempoSirene > 20000) {
      tempoSirene = 0;
    }
  }
}

//----------FONCTIONS----------//

//Fonction qui change l'état de l'alarme sur l'écran LCD
void ecranEtatAlarme() {
  if (alarme) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("ACTIVEE");
    lcd.setCursor(0, 1);
    lcd.print("SAISIR CODE:");
  }
  else {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("DESACTIVEE");
    lcd.setCursor(0, 1);
    lcd.print("SAISIR CODE:");
  }
}

//Fonction qui affiche un écran de confirmation lorsqu'on valide le code
void ecranValiditeCode() {
  if (code) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("---CORRECT---");
  }
  else {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("---INCORRECT---");
  }
}

//Fonction qui change l'état de la LED statut en fonction de l'état de l'alarme
void etatLed() {
  if (alarme) {
    digitalWrite(led, HIGH);
  }
  else {
    digitalWrite(led, LOW);
  }
}

//Fonction qui permet de réinitialiser le code et l'effacer de l'écran LCD
void reinitialiserCode() {
  motDePasse.reset();
  ecranEtatAlarme();
  posLCD = 12;
}

//Fonction qui permet de tester si le code entré par l'utilisateur et correct
//Si c'est le cas, on change l'état de l'alarme (ACTIVEE->DESACTIVEE ou DESACTIVEE->ACTIVEE)
//On met à jour l'état de la led
//On réinitialise le code ainsi que les variables de contrôle
void verifierCode() {
  if (motDePasse.evaluate()) {
    code = true;
    ecranValiditeCode();
    delay(1000);
    alarme = !alarme;
    etatLed();
    reinitialiserCode();
    reinitialiserVariablesControle();
  }
  else {
    code = false;
    ecranValiditeCode();
    delay(1000);
    reinitialiserCode();
  }
}

//On ajoute le chiffre sur lequel on a appuyé au code entré (concaténation)
//On l'affiche sur l'écran LCD
void ajouter() {
  motDePasse.append(bouton);
  lcd.setCursor(posLCD, 1);
  lcd.print(bouton);
  posLCD = posLCD + 1;
  if (posLCD > 15) {
    posLCD = 12;
  }
}

//Fonction qui permet de réinitialiser les variables de contrôle
void reinitialiserVariablesControle() {
  etape = 1;
  detection = false;
  timeout = false;
  tempsMort = true;
  compteur = 0;
}
