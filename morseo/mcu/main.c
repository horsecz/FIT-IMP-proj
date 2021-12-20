/*******************************************************************************
   main: main for morseo
   Author(s): Dominik Horky <xhorky32 AT stud.fit.vutbr.cz>

   LICENSE TERMS

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:
   1. Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
   2. Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the
      distribution.
   3. All advertising materials mentioning features or use of this software
      or firmware must display the following acknowledgement:

        This product includes software developed by the University of
        Technology, Faculty of Information Technology, Brno and its
        contributors.

   4. Neither the name of the Company nor the names of its contributors
      may be used to endorse or promote products derived from this
      software without specific prior written permission.

   This software or firmware is provided ``as is'', and any express or implied
   warranties, including, but not limited to, the implied warranties of
   merchantability and fitness for a particular purpose are disclaimed.
   In no event shall the company or contributors be liable for any
   direct, indirect, incidental, special, exemplary, or consequential
   damages (including, but not limited to, procurement of substitute
   goods or services; loss of use, data, or profits; or business
   interruption) however caused and on any theory of liability, whether
   in contract, strict liability, or tort (including negligence or
   otherwise) arising in any way out of the use of this software, even
   if advised of the possibility of such damage.

   $Id$


*******************************************************************************/

#include <fitkitlib.h>
#include <lcd/display.h>
#include <keyboard/keyboard.h>
#include <string.h>
#include <stdbool.h>


/*******************************************************************************
 * definice konstant
*******************************************************************************/
#define LCD_MAX_CHARS 32									/**< maximalni pocet znaku na displeji (FITKit 2.0) **/
#define RESULT_MAX_PAGES 25									/**< nejvetsi pocet 'stran' pro zpravu - limitovano pameti FITKitu **/
#define RESULT_MAX_CHARS LCD_MAX_CHARS*RESULT_MAX_PAGES		/**< maximalni pocet znaku zpravy **/
#define SYMBUFF_MAX 5										/**< nejvetsi pocet symbolu morseovky za sebou pro jeden znak **/


/*******************************************************************************
 * preddefinice funkci, nutne pro funkce, ktere se pouzivaji drive, nez jsou deklarovane
*******************************************************************************/
void handle_keys();
void charToDisplay();
void LCD_clearme();
void LCD_restart();
void clearSymBuff();
void movePage(bool up);


/*******************************************************************************
 * GLOBALNI PROMENNE
*******************************************************************************/
// prepinace
bool start = true;				/**< Indikuje prvni spusteni **/
bool testMode = false;			/**< Indikator stavu testovani (nastaveni -> zmena delky stisku -> test) **/
bool lenSetting = false;		/**< Indikator stavu nastaveni delky stisku **/
bool ignoreInput = false;		/**< V pripade, ze kit je ve stavu nastaveni, nebude prijiman vstup z tlacitka nuly **/
bool firstChar = true;			/**< Indikuje, ze uzivatel zadava uplne prvni posloupnost symbolu morseovky (=> prvni znak, nutno cistit displej) **/
bool outOfBonds = false;		/**< Indikuje, ze pole symbolu morseovky je plne a je nutne 'odeslat' znak, prevest a zobrazit na displej **/
bool viewMode = false;			/**< Indikator stavu prohlizeni zpravy **/
bool msgRemoval = false;		/**< Indikator stavu, kdy uzivatel chce smazat vyslednou zpravu **/

// pocitadla
unsigned int cnt = 0;				/**< pocitadlo delky stisku tlacitka **/
unsigned int KPR = 5;				/**< nasobic delky stisku, standardni je 5 **/
unsigned int lcd_char_counter = 0;  /**< pocet znaku na displeji **/

// buffery, ukazovatka, citace
char symBuff[SYMBUFF_MAX+2];					/**< buffer pro symboly morseovky pro jeden znak **/
char displayText[LCD_MAX_CHARS];				/**< buffer soucasne zobrazovaneho textu na displeji **/
char resultBuff[LCD_MAX_CHARS*RESULT_MAX_PAGES];/**< buffer vysledneho textu **/
int lCode = 0;									/**< ukazovatko-index posledniho prazdneho znaku v symBuff **/
int finalCnt = 0;								/**< ukazovatko-index posledniho prazdneho znaku v resultBuff **/
int pageCnt = 0;								/**< citac poctu stranek textu **/
int currPage = 0;								/**< aktualni zobrazena stranka **/
const char *morseCodes[] = { 					/**< konstantni seznam platnych posloupnosti morseovy abecedy **/
	".-", "-...", "-.-.", "-..", ".",		//A-E
	"..-.", "--.", "....", "..", ".---",	//F-J
	"-.-", ".-..", "--", "-.", "----",		//K-O
	".--.", "--.-", ".-.", "...", "-",		//P-T
	"..-", "...-", ".--", "-..-", "-.--",	//U-Y
	"--..",									//Z
	
	"-----", ".----", "..---", "...--",		//0-3
	"....-", ".....", "-....", "--...",		//4-7
	"---..", "----.",						//8-9
};


/*******************************************************************************
 * FUNKCE
*******************************************************************************/
/*******************************************************************************
 * Konverze posloupnosti symbolu Morseovy abecedy na znaky latinky
 * @param morseCode posloupnost symbolu kratkych a dlouhych signalu
 * @return znak latinky, cislice nebo 0, pokud jde o neplatny znak
*******************************************************************************/
char convertMorseToChar (char* morseCode) {
	char result = 0;
	int k = -1;
	int i;	// jelikoz QDevKit nepracuje s C99+, je nutne deklarovat pocitadlo pred cyklem
	if (!strlen(morseCode)) {
		term_send_str_crlf("<nic> -> <mezera>");
		return ' ';
	}
	term_send_str(morseCode);
	term_send_str(" -> ");
	for (i = 0; i < 36; i++) {
		if (!strcmp(morseCode, morseCodes[i])) {
			k = i;
			break;
		}
	}
	if (k < 26 && k >= 0) {
		result = 'A' + k;
	} else if (k >= 26) {
		result = '0' - (26 - k);
	} else {
		result = '?';
	}
	term_send_char(result);
	term_send_crlf();
	return result;	
}


/*******************************************************************************
 * Vypis uzivatelske napovedy (funkce se vola pri vykonavani prikazu "help")
*******************************************************************************/
void print_user_help(void)
{
  term_send_str_crlf(" Tato aplikace nepodporuje zadne dalsi prikazy v terminalu.");
  term_send_str_crlf("----------------------------------------------------");
}


/*******************************************************************************
 * Dekodovani a vykonani uzivatelskych prikazu
*******************************************************************************/
unsigned char decode_user_cmd(char *cmd_ucase, char *cmd)
{
	return (CMD_UNKNOWN);
}


/*******************************************************************************
 * Inicializace periferii/komponent po naprogramovani FPGA
*******************************************************************************/
void fpga_initialized() 
{
  term_send_str_crlf("--- IMP Projekt ------------------------------------");
  term_send_str_crlf("--- Dekoder Morseovy abecedy -----------------------");
  term_send_str_crlf("----------------------------------------------------");
  term_send_str_crlf("Text v Morseove abecede zadavejte pomoci tlacitka '0'.");
  term_send_str_crlf("Pro napovedu zadejte v tomto terminalu prikaz 'help'.");
  term_send_str_crlf("----------------------------------------------------");

  LCD_init();                          // inicializuj LCD
  LCD_clear();
  LCD_append_string("Zadejte kod  v  Morseove abecede");   // zobraz text na LCD
  LCD_send_cmd(LCD_DISPLAY_ON, 0);
}


/*******************************************************************************
 * Prevede uroven rychlosti stisku na text
*******************************************************************************/
void KPRtoText(char* textBuffer) {
	switch (KPR) {
		case 5:
			strcpy(textBuffer,"OPTIMAL");
			break;
		case 3:
			strcpy(textBuffer,"KRATKY");
			break;
		case 7:
			strcpy(textBuffer,"STREDNI");
			break;
		case 10:
			strcpy(textBuffer,"DLOUHY");
			break;
	}
}


/*******************************************************************************
 * Nastavi KPR dle stisknuteho tlacitka
*******************************************************************************/
void setKPR(char ch) {
	switch(ch) {
		case 'A':
			KPR = 5;
			break;
		case 'B':
			KPR = 3;
			break;
		case 'C':
			KPR = 7;
			break;
		case 'D':
			KPR = 10;
			break;
	}
}


/*******************************************************************************
 * Hlavni funkce pro obsluhu tlacitek kitu
*******************************************************************************/
void handle_keys() {
  char ch;
  ch = key_decode(read_word_keyboard_4x4());

  switch (ch) {
	case '0':		// stisknuta nula -> zadava se symbol morseovky
		start = false;
		cnt++;
		if (!ignoreInput) set_led_d5(1);
		break;
	case '#':		// mrizka -> nastaveni delky stisku
		if (!lenSetting) {		// vstup do modu nastaveni
			if (msgRemoval && !viewMode) {
				msgRemoval = false;
				LCD_clearme();
				LCD_append_string("Zprava zanechana");
				delay_ms(2500);
				LCD_clearme();
				LCD_restart();
				break;
			}
			if (viewMode) break;

			lenSetting = true;
			ignoreInput = true;
			char setting[10];
			KPRtoText(setting);

			LCD_clearme();
			LCD_append_string("Nastaveni delky stisku: ");
			LCD_append_string(setting);
			term_send_str_crlf("Zmenen mod na: NASTAVENI DELKY STISKU");
		} else {				// vstup do modu dekodovani vstupu, opusteni nastaveni
			lenSetting = false;
			firstChar = true;
			testMode = false;
			ignoreInput = false;
			LCD_clearme();
			LCD_restart();
		}
		delay_ms(1000);
		break;
	case '*':
		if (msgRemoval && !viewMode) {	// potvrzeni smazani zpravy
			msgRemoval = false;
			int i = 0;
			for (i=0; i < RESULT_MAX_CHARS; i++) { resultBuff[i] = '\0'; }
			finalCnt = 0;
			LCD_clearme();
			LCD_append_string("Zprava smazana");
			delay_ms(2500);
			LCD_clearme();
			LCD_restart();
			break;
		}
		if (!lenSetting && !testMode && !viewMode)	// defaultni cinnost hvezdicky
			charToDisplay();
		delay_ms(500);
		break;
	case 'A':	// pismeno A posouva v modu pro prohlizeni zpet (stranka--)
		if (!lenSetting && viewMode) {
			movePage(false);
			LCD_restart();
			delay_ms(1000);
			break;
		}
	case 'B': 	
		if (!lenSetting && viewMode) { // pri prohlizeni stranka++
			movePage(true);
			LCD_restart();
			delay_ms(1000);
			break;
		} else if (!lenSetting && !viewMode && ch == 'B') { // pokud neprohlizim, slouzi jako backspace
			finalCnt--;
			if (finalCnt < 0) { finalCnt = 0; }
			resultBuff[finalCnt] = '\0';
			pageCnt = finalCnt / LCD_MAX_CHARS;
			currPage = pageCnt;
			LCD_clearme();
			LCD_restart();
			delay_ms(1000);
			break;
		}
	case 'C':
		if (!lenSetting && !viewMode && ch == 'C') { // clear -> budto maze buffer symbolu morseovky, nebo pokud je buffer prazdny, tak maze dekodovanou zpravu
			if (!strlen(symBuff)) {
				if (!strlen(resultBuff)) break;
				LCD_clearme();
				LCD_append_string("Smazat zpravu?  A: * ; N: #");
				msgRemoval = true;
				delay_ms(100);
				break;
			} else {
				clearSymBuff();
				LCD_clearme();
				LCD_append_string("Buffer symbolu  vyprazdnen");
				delay_ms(2000);
				LCD_clearme();
				LCD_restart();
				break;
			}
		}
	case 'D':
		if (lenSetting) {	// pokud je stisknuto jedno z techto tlacitek (A,B,C,D) a kit je v modu nastaveni -> zobrazi docasne zpravu o zmene nastaveni a pote prejde do test modu 
			setKPR(ch);
			char setting[10];
			KPRtoText(setting);
			LCD_clearme();
			LCD_append_string("Zmenena delka na ");
			LCD_append_string(setting);
			
			testMode = true;
			ignoreInput = false;
			delay_ms(2000);
			lcd_char_counter = 6;
			LCD_clear();
			LCD_append_string("Test: ");
		} else if (ch == 'D') { // specialni interakce jen pro D -> vypne a zapne viewMode
			if (viewMode) {	
				term_send_str_crlf("vypnut view mod");
				viewMode = false;
				ignoreInput = false;
				LCD_clearme();
				LCD_append_string("Prohlizeni      VYPNUTO");
				delay_ms(2000);
				LCD_clearme();
				LCD_restart();
			} else {
				currPage = 0;
				term_send_str_crlf("zapnuty view mod");
				viewMode = true;
				ignoreInput = true;
				LCD_clearme();
				LCD_append_string("Prohlizeni      ZAPNUTO");
				delay_ms(2000);
				LCD_clearme();
				LCD_restart();

			}
		break;
		}
  	}
}


/*******************************************************************************
 * Kratky stisk nuly -> zaznamenava se '.' (reprezentuje kratky signal)
*******************************************************************************/
void short_press() {
	if (testMode) {
		LCD_append_string(".");
	} else {
	if (firstChar) { LCD_clear(); firstChar = false; }
	if (!outOfBonds)
		symBuff[lCode++] = '.';
	}
}


/*******************************************************************************
 * Dlouhy stisk nuly -> zaznamenava se '-' (reprezentuje dlouhy signal)
*******************************************************************************/
void long_press() {
	if (testMode) {
		LCD_append_string("-");
	} else {
	if (firstChar) { LCD_clear(); firstChar = false; }
	if (!outOfBonds)
		symBuff[lCode++] = '-';
	}
}


/*******************************************************************************
 * Vyprazdni buffer symbolu morseovky
*******************************************************************************/
void clearSymBuff() {
	int i = 0;
	for (i=0; i<6; i++) { symBuff[i] = '\0'; }
	lCode = 0;
}


/*******************************************************************************
 * Prida znak do bufferu vysledne zpravy, aktualizuje pocet stran, posune
 * aktualni stranu v pripade zmeny (resp. pokud je zaplneny displej)
*******************************************************************************/
void addCharToResult(char x) {
	resultBuff[finalCnt++] = x;
	if (finalCnt > RESULT_MAX_CHARS) {
		LCD_clearme();
		LCD_append_string("Nelze zapsat dalsi znak.");
	}
	pageCnt = finalCnt / LCD_MAX_CHARS;
	currPage = pageCnt;
}


/*******************************************************************************
 * Vypise jednu (soucasnou) stranku zpravy na displej FITKitu
*******************************************************************************/
void showResult() {
	int i = 0;
	for (i=0; i < LCD_MAX_CHARS; i++) {
		displayText[i] = resultBuff[currPage*LCD_MAX_CHARS + i];
	}
	LCD_clear();
	LCD_append_string(displayText);
}


/*******************************************************************************
 * Zmeni aktualni stranu
 * @param up pokud je true, prohlizi se nasledujici strana, pokud false, tak predchozi
*******************************************************************************/
void movePage(bool up) {
	if (up) {
		currPage++;
		if (currPage > pageCnt)
			currPage = 0;
	} else {
		currPage--;
		if (currPage < 0)
			currPage = pageCnt;
	}
}


/*******************************************************************************
 * Prelozeny znak se ulozi do bufferu vysledneho textu a zobrazi na displeji
*******************************************************************************/
void charToDisplay() {
	char x = convertMorseToChar(symBuff);
	set_led_d6(0);
	lcd_char_counter++;
	addCharToResult(x);
	showResult();
	clearSymBuff();
	outOfBonds = false;
	delay_ms(100);
}


/*******************************************************************************
 * Vyprazdni obsah na displeji
*******************************************************************************/
void LCD_clearme() {
	LCD_clear();
	lcd_char_counter = 1;
}


/*******************************************************************************
 * "Restartuje" displej (vycisti a ukaze vysledny text nebo uvodni text)
*******************************************************************************/
void LCD_restart() {
	LCD_clearme();
	if (finalCnt)
		showResult();
	else {
		if (viewMode) {
			LCD_append_string("<zadny text>");
		} else {
			LCD_append_string("Zadejte kod  v  Morseove abecede");
		}
	}
}


/*******************************************************************************
 * Kontrola validity pole symbolu morseovky
*******************************************************************************/
void symBuffCheck() {
	if (lCode > 5) { 
		set_led_d6(1); // rozsviti diodu D6 (cervena)
		term_send_str_crlf("Neukonceny znak"); 
		outOfBonds = true;
	} else {
		term_send_str("symBuff: ");
		term_send_str_crlf(symBuff);
	}
}


/*******************************************************************************
 * Hlavni funkce
*******************************************************************************/
int main(void) 
{
	unsigned int last_cnt = 0;	// pocitadlo pro stisk tlacitka

  	// inicializace
  	initialize_hardware();
  	keyboard_init();

  	WDG_stop();
  	CCTL0 = CCIE;
  	TACTL = 0;

  	// hlavni smycka
  	while (1) {
    	handle_keys();
		if (!start) { // pokud se nenachazim na pocatecnim stavu
			if (last_cnt != cnt) {
				last_cnt = cnt;
			} else {
				if (cnt > 0 && !ignoreInput) { 		// pokud je tlacitko stisknuto a zaroven nejsem v nastaveni, kdy je potreba ignorovat vstup
					lcd_char_counter++; 			// znak na displeji ++
					if (lcd_char_counter > LCD_MAX_CHARS-1) LCD_clearme(); // pokud je displej zcela zaplnen, vyprazdni

					// rozeznani delky stisku a kontrola bufferu symbolu
					if (cnt < 1000*KPR) short_press();
					else long_press(); 
					if (!testMode) symBuffCheck();
				}
			
				// vypne diodu D5 (tlacitko uz neni stiskle) a restartuje pocitadla stisku
				set_led_d5(0);
				cnt = 0;
				last_cnt = 0;
			}
		}
    	terminal_idle();	// obsluha terminalu
  	} 
}
