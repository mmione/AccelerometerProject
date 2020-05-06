                                                                                              // filename ******** Main.C ************** 

//***********************************************************************
// Simple ADC example for the Technological Arts EsduinoXtreme board
// by Carl Barnes, 12/03/2014
//***********************************************************************

#include <hidef.h>      /* common defines and macros */
#include "derivative.h"  /* derivative information */
#include "SCI.h"

char string[20];
char char1;   //need to split these up to transmit!
char char2;

////defining function prototypes

void msDelay(unsigned int);
void setClk(void);
int arcSin(int arg);
void BCDOutput(int angle);
void BarOutput(int angle);
void clearDisplay(void);

int negativeFlag;
int mode;
  
int tempAngle;
int valueRead;
unsigned int maskVal;

int buttonCounter;

//---------------------OutCRLF---------------------
// Output a CR,LF to SCI to move cursor to a new line
// Input: none
// Output: none
// Toggle LED each time through the loop

void OutCRLF(void){
  SCI_OutChar(CR);
  SCI_OutChar(LF);
  PTJ ^= 0x20;          // toggle LED D2
}

void main(void) {

  // ANALOG-DIGITAL SETUP //////////////

		
	ATDCTL1 = 0x4F;		// set for 12-bit resolution - ((AS PER SPEC!!))
	
	//not sure 
	ATDCTL3 = 0x88;		// right justified, one sample per sequence

	ATDCTL4 = 0x02;		// prescaler = 2; ATD clock = 8MHz / (2 * (2 + 1)) == 1.33MHz
	ATDCTL5 = 0x26;		// continuous conversion on channel 0
	
  ////////////////////////////////////////////////////////////
  
  TSCR1 = 0x90;	//Timer System Control Register 1
                    // TSCR1[7] = TEN:  Timer Enable (0-disable, 1-enable)
                	// TSCR1[6] = TSWAI:  Timer runs during WAI (0-enable, 1-disable)
                	// TSCR1[5] = TSFRZ:  Timer runs during WAI (0-enable, 1-disable)
        	        // TSCR1[4] = TFFCA:  Timer Fast Flag Clear All (0-normal 1-read/write clears interrupt flags)
                	// TSCR1[3] = PRT:  Precision Timer (0-legacy, 1-precision)
                	// TSCR1[2:0] not used
 
  TSCR2 = 0x06;	//Timer System Control Register 2   was 0x04
                	
 
                	
  TIOS = 0xFC; 	//set PT0 AND PT1 to be inputt capture!
                	
  PERT = 0x03; 	//Enable Pull-Up resistor on TIC[0] TIC[1]
 
  TCTL3 = 0x00;	//TCTL3 & TCTL4 configure which edge(s) to capture
  
  TCTL4 = 0b00001010;	//Configured for falling edge on TIC[0]   and TIC[1]
 

  TIE = 0x01;  	//Timer Interrupt Enable
          
  IRQCR=0x00;
	EnableInterrupts;      //now we will catch interrupts!!
  
  //E-CLOCK STUFF!

  setClk();    // set to 8MHz

  ////////////////////////////////////////////////////////////

  //GENERAL SETUP

  DDRJ = 0x01;     // PortJ bit 0 is output to LED D2 on DIG13
  
  DDRP=0x1F;
  
  //PAD SETUP//

  DDR1AD=0x00; ; //A6, A0, A1 configured for input     (as well as all others!)
  PER1AD=0x01;  ///ENABLE pull up resistors
  
  ATDDIEN=0b10111100;  //NEED this step        
  
  valueRead=0;
  maskVal=0;
  
  negativeFlag=0;
  mode=0;
  
  SCI_Init(9600);
  
  SCI_OutString("DONE SETUP! "); OutCRLF();
  
  //CONSTANTLY executing!
  
  
  
  
  
  for(;;) {
    
    SCI_OutUDec(PT1AD);
    SCI_OutUDec(PTI1AD);
    OutCRLF();
    
  }
}

///// other functions /////

interrupt  VectorNumber_Vtimch0 void ISR_Vtimch0(void)
{
  
  
  unsigned int temp; //this wacky stuff NEEDS to be here, or we'll have an infinite loop.
  mode = (mode +1)%3; 
  PTJ ^= 0x01;
  temp = TC0; 
  
  SCI_OutString("interrupt triggered ");
  SCI_OutString("Mode: ");
  OutCRLF();
  SCI_OutUDec(mode);
  OutCRLF();
  
  
  
  return;
  
} 


void msDelay(unsigned int time)      //this method has been FIXED!!
{
	unsigned int j,k;
	for(j=0;j<time;j++)
		for(k=0;k<1323;k++);    //INITIALLY, this value was k<1033, but we changed from 6.25 clk to 8, 1.28x incr, so 
		                        //we get 1323
}


void setClk(void){
  
  CPMUPROT = 0x26;               // EITHER ALLOWS YOU TO CHANGE or NOT CHANGE your cpu registers. 
  
								 
  CPMUCLKS = 0x80;               //We choose to base the bus clock on the PLL (phase-locked loop!!!!!)
  
  
  
  CPMUOSC = 0x80;                //We ENABLE (external) oscillator! (OSCE=1).PLLclk will use this later, as:fOSC (8 MHz)
  
 
  
  CPMUREFDIV = 0x41;   //we set REFDIV to be equal to 1, and get f_ref = f_osc/(REFDIV+1) = 8/(1+1) = 4, this fits with the frequency range we choose with this register TOO, 2<f<6
  
  
  
  CPMUSYNR= 0x05;      //Set Syn divide and selects VCO frequency range. fVCO = 48 MHz.
  
  CPMUPOSTDIV=0x02;              //POST DIVIDER = 2, and  fPLL= 48/(2+1) MHz = 16
                                
  //Overall clock speed is fBUS=fPLL/2=8 MHz, we GOOD!!
  
  //now, we wait for the PLL to engage: not needed when the oscillator is our reference
  
  while (CPMUFLG_LOCK == 0) {}  
  
  CPMUPROT = 1;                  //Protection for clock configuration is reenabled 
  
  //NOTE: current delay function is updated to properly delay!
  
}

int arcSin(int arg){
 
   double argDouble = (double) arg;
   
   double result;
   double midValue=2840.0;
   double rangeToMid=280;         //see if this changes the accuracy, was 280.0
   double temp;
   
   int q=0;
   
   //SCI_OutString("Value read: ");
   //SCI_OutUDec(valueRead);
   //OutCRLF();  
   
   if(arg<2850 && arg>=2835){
     negativeFlag=0;
     q=0;
     //SCI_OutString("Angle: (degrees) ");
     SCI_OutUDec(0);
     //msDelay(500);
     OutCRLF();
     
   } else if(arg>=2850){          //angle is now POSITIVE!
   
     negativeFlag=0;
     
     temp= (argDouble-midValue)/rangeToMid;             //gets a ratio of possible POS distance
     
     result = temp + (1/6)*temp*temp*temp + (3/40)*temp*temp*temp*temp*temp + (15/336)*temp*temp*temp*temp*temp*temp*temp;    //this is a taylor series approx of arcsin;
     
     result *=90;        //multiplies our RATIO to get degrees. 
     
     q=(int)result;
     
     if(arg>3115){
      
      q=90;    //we have exceeded the range of NORMAL values, we must set to 90.
     
     }
     
     //SCI_OutString("Angle: (degrees) ");
     SCI_OutUDec(q);
     OutCRLF();
     //msDelay(500);
     
     } else if(arg<2835){    //angle is now NEGATIVE!
      
      negativeFlag=1;
    
     //now will implement the reverse function
     
     if(arg<=2565){      //we've hit the threshold
      
      q=90;
      
     }
     
     temp = (midValue-argDouble)/rangeToMid;
     
     result = temp + (1/6)*temp*temp*temp + (3/40)*temp*temp*temp*temp*temp + (15/336)*temp*temp*temp*temp*temp*temp*temp;    //this is a taylor series approx of arcsin;
     
     result*=90; //if we were to multiply by a NEGATIVE OutUDec would throw an overflow...
     
     q=(int)result;
    
     //SCI_OutString("Angle: (negative) (degrees) ");
     SCI_OutUDec(q);
     OutCRLF();
     //msDelay(500);
   } else {
      
     //SCI_OutString("oops");
   }
   
   return q;
}

void BCDOutput(int angle){
  
  int tens = angle/10;
  int ones = angle%10;  //extracts the ones and tens from the angle given
 
  /*OutCRLF();
  SCI_OutString("TENS: ");        //debug stuff!!
  SCI_OutUDec(tens);
  OutCRLF();
  SCI_OutString("ONES: ");
  SCI_OutUDec(ones);     */
  
  clearDisplay();
  
  if(negativeFlag==1){
  
    PTP = PTP | 0b00010000;
    
  } else if(negativeFlag==0){
   
    PTP = PTP & 0b11101111;
    
  }
  
  
  
  if(ones==0){
   
    PT1AD=PT1AD & 0b11000011; 
   
  } else if(ones==1){
   
    PT1AD=PT1AD | 0b00000100;
    
  } else if(ones==2){
   
    PT1AD=PT1AD | 0b00001000;
   
  } else if(ones==3){
   
    PT1AD=PT1AD | 0b00001100;
    
  } else if(ones==4){
    
    PT1AD=PT1AD | 0b00010000;
  
  } else if(ones==5){
    
    PT1AD=PT1AD | 0b00010100;
    
  } else if(ones==6){
  
    PT1AD=PT1AD | 0b00011000;
    
  } else if(ones==7){
    
    PT1AD=PT1AD | 0b00011100;
  
  } else if(ones==8){
   
   
    PT1AD=PT1AD | 0b00100000; 
    
  } else {
   
    PT1AD=PT1AD | 0b00100100;
    
  }
  
  //tens
  
  if(tens==0){
   
    PTP= PTP & 0b11100000; 
   
  } else if(tens==1){
   
    PTP = PTP | 0b00000001;
    
  } else if(tens==2){
   
    PTP = PTP | 0b00000010;
   
  } else if(tens==3){
   
    PTP = PTP | 0b00000011;
    
  } else if(tens==4){
    
    PTP = PTP | 0b00000100;
  
  } else if(tens==5){
    
    PTP = PTP | 0b00000101;
    
  } else if(tens==6){
  
    PTP = PTP | 0b00000110;
    
  } else if(tens==7){
    
    PTP = PTP | 0b00000111;
  
  } else if(tens==8){
   
   
    PTP = PTP | 0b00001000; 
    
  } else {
   
    PTP = PTP | 0b00001001;
    
  }
  
  
  
  return;
  
}

void BarOutput(int angle){
  
 //need to divide the angle into chunks
 
 //8 bits to rep 0-90, thus each led is 11.25
 
 if(negativeFlag==1){ //these statements can be the ONLY ones to modify the negative led
 
    PTP= PTP | 0b00010000;  //sets it hi!
 
 } else if(negativeFlag==0){
  
    PTP = PTP & 0b11101111;
  
 }
 /////
 
 
 if(angle>0 && angle<11){
  
   //all leds off, want to set from L --> R
  
    PTP = PTP & 0b00010000;;  //never wanna mess with 
    PT1AD=PT1AD & 0b11000011;
  
 } else if(angle>=11 && angle<23){
  
    //leftmost led on
    
    BCDOutput(80);
  
 } else if(angle>=23 && angle<34){
  
    //2 leds on
    
    PTP = PTP | 0b00001100;
    PT1AD=PT1AD & 0b11000011; 
  
 } else if(angle>=34 && angle<45){
    
    //3 on
    PTP = PTP | 0b00001110;
    PT1AD=PT1AD & 0b11000011;
    
 
 } else if(angle>=45 && angle<57){
  
   PTP = PTP | 0b00001111;
   PT1AD=PT1AD & 0b11000011; 
  
 } else if(angle>=57 && angle<68){
  
   PTP = PTP | 0b00001111;
   PT1AD=PT1AD | 0b00100000; 
  
 } else if(angle>=68 && angle<79){
  
  
   PTP = PTP | 0b00001111;
   PT1AD=PT1AD | 0b00110000; 
   
 } else if(angle>=79 && angle<89){
   
   PTP = PTP | 0b00001111;
   PT1AD=PT1AD | 0b00111000; 
   
 } else if(angle>=90){
    
   PTP = PTP | 0b00001111;
   PT1AD=PT1AD | 0b00111100; 
 }
  
}

void clearDisplay(){
 
  PTP=PTP & 11100000;
  PT1AD= PT1AD & 11000011;
  return;
}