#include <iostream>
#include <fstream>
#include <iomanip>

using namespace std;

extern "C"{
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <jvme.h>
#include <tiLib.h>
#include <vmeDSClib.h>
#include "math.h"
#include "time.h"   
#include "sys/types.h"
#include <unistd.h>

  extern int tdID[21];
  extern int nTD;
  extern unsigned int dscA32Base;
  
  DMA_MEM_ID vmeIN,vmeOUT;
  extern DMANODE *the_event;
  extern unsigned int *dma_dabufp;
  
}


unsigned int ScalerData[19][16];

int NDISC = 12;
int START_SLOT = 4;


int THRESHOLD = 20;
int PULSEWIDTH = 20;
int VIEWONLY = 0;
int NORMAL = 1;
int SPECIAL = 0;

int DecodeData(DMANODE *outEvent);

int  main(int argc, char *argv[])  {


  for (int k=0;k<argc;k++){
    
    if (!strcmp(argv[k],"-T")){
      THRESHOLD = atoi(argv[++k]);
    }
    if (!strcmp(argv[k],"-W")){
      PULSEWIDTH = atoi(argv[++k]);
    }
    if (!strcmp(argv[k],"-V")){
      VIEWONLY = 1;;
      NORMAL = 0;
    }
    if (!strcmp(argv[k],"-S")){
      VIEWONLY = 0;
      NORMAL = 0;
      SPECIAL = 1;
      THRESHOLD = atoi(argv[++k]);
      PULSEWIDTH = 20;
    }
    
  }



  int dCnt;
  vmeOpenDefaultWindows();

  vmeDmaConfig(2,5,1);

  dmaPFreeAll();
  vmeIN  = dmaPCreate("vmeIN",1024*4000,5,0);
  // vmeIN  = dmaPCreate("vmeIN",10244,500,0);
  vmeOUT = dmaPCreate("vmeOUT",0,0,0);
  dmaPStatsAll();

  dmaPReInitAll();
   

  printf("--- SET UP VME DISCRIMINATOR ---\n");

  vmeDSCInit((START_SLOT<<19), (1<<19), NDISC, 0);

  if (NORMAL){
    for(int idsc=0; idsc<NDISC; idsc++) {
      int DSC_SLOT = vmeDSCSlot(idsc);
      vmeDSCClear(DSC_SLOT);
      vmeDSCSetGateSource(DSC_SLOT, 1, DSC_GATESRC_CONSTANT);
      vmeDSCSetGateSource(DSC_SLOT, 2, DSC_GATESRC_CONSTANT);
      
      vmeDSCReadoutConfig(DSC_SLOT, 
			  DSC_READOUT_TRG_GRP1 | DSC_READOUT_REF_GRP1 | 
			  DSC_READOUT_LATCH_GRP1, 
			  DSC_READOUT_TRIGSRC_SOFT);
    }
    vmeDSCSetThresholdAll(THRESHOLD, THRESHOLD);
    vmeDSCSetPulseWidthAll(PULSEWIDTH, PULSEWIDTH, 1);
    vmeDSCGStatus(0);
  } else if (VIEWONLY) {

     for(int idsc=0; idsc<NDISC; idsc++) {
      int DSC_SLOT = vmeDSCSlot(idsc);
      int val1,val2;
      for (int n=0; n<16; n++) {
	val1 = vmeDSCGetThreshold(DSC_SLOT, n, 1);
	val2 = vmeDSCGetThreshold(DSC_SLOT, n, 2);
	printf("SLOT %d   CHAN %02d: Threshold at %d / %d\n", DSC_SLOT, n, val1, val2);
      }
      // vmeDSCStatus(DSC_SLOT,1);
     }
  }

  std::cout<<"....conintue?(hit return) ";
  getchar();
  
  while(1){

    sleep(10);
    printf("slept 10");

    memset(ScalerData, 0, 19*16*4);

    GETEVENT(vmeIN,0);

    for (int k=0;k<NDISC;k++){

      int DSC_SLOT = vmeDSCSlot(k);
      int ready=10000;

      vmeDSCSoftTrigger(DSC_SLOT);

      /* Check for data ready */
      for(int i=0; i<100; i++)
        {
          ready = vmeDSCDReady(DSC_SLOT);
          if(ready)
            break;
        }

      if(ready) {

	//printf("%02d: Ready = %d\n",DSC_SLOT,readypwd);
	dCnt = vmeDSCReadBlock(DSC_SLOT, dma_dabufp, 100, 1);
	if (dCnt>0) {
	  printf("%2d: dCnt = %d\n",DSC_SLOT,dCnt);
	  dma_dabufp += dCnt;
	} else{
	  printf("ERROR: No data from DSC ID %02d\n",DSC_SLOT);
	}
      } else {
	printf("ERROR: Data not ready from DSC ID %02d\n",DSC_SLOT);
      }
      
    }

    
    PUTEVENT(vmeOUT);
    DMANODE *outEvent = dmaPGetItem(vmeOUT);
    
    printf("outevent length = %d\n",outEvent->length);


    /*
    for( int idata=0;idata<outEvent->length;idata++)
      {
	if((idata%4)==0) printf("\n\t");
	printf("  0x%08x ",(unsigned int)LSWAP(outEvent->data[idata]));
      }
    printf("\n\n");
    */


    int nchan = DecodeData(outEvent);

    char w[128];
    
    for (int i=0; i<NDISC; i++){
      sprintf(w,"Slot%d",i+START_SLOT);
      std::cout<<std::setw(10)<<w;
    }
    std::cout<<endl;

    for (int k=0; k<16; k++) {

      for (int i=0; i<NDISC; i++ ){
	std::cout<<std::setw(10)<<ScalerData[i][k];
      }
      std::cout<<std::endl;
    }
    
    
    
    dmaPFreeItem(outEvent);
    
    

  }



  dmaPFreeAll();
  vmeCloseDefaultWindows();



  return 0;
}


int DecodeData(DMANODE *outEvent){

  int NumberOfChannels = 0;
  int SLOTID;
  int TRIGGERNUMBER;
  int CHANID;
  int IN2;
  int IN1;
  int BUILDERFLAG;
  int SCALERLENGTH;
  int DATANOTVALID;

  for( int idata=0;idata<outEvent->length;idata++){
    
    unsigned int wrd = LSWAP(outEvent->data[idata]);

    if( wrd & 0x80000000 ){ /* data type defining word */
      
      unsigned int dataType = (wrd & 0x78000000) >> 27;
      
      switch(dataType){
        
        
      case 0: /*BLOCK HEADER*/
        break;
        
        
      case 1:/* BLOCK TRAILER */
        break;
        
      case 2:/* Event Header */
        SLOTID = (wrd & 0x07c00000) >> 22;
        TRIGGERNUMBER = (wrd & 0x3FFFFF);
        CHANID=0;
        break;
        
      case 4:/* Scaler Header */
        IN2 = (wrd & 0x20000)>> 8;
        IN1 = (wrd & 0x10000)>> 8;
        BUILDERFLAG = (wrd & 0x3F00)>> 8;
        SCALERLENGTH = (wrd & 0xFF); 
        //printf("SLOT %d : BuilderFlag is 0x%x \n",SLOTID,BUILDERFLAG);
        break;
        
      case 14:/* Data Not Valid */
        DATANOTVALID = 1;
        break;
        
      case 15:/* Filler Word */
        DATANOTVALID = 1;
        break;
        
      }
    
    } else{

      /* data word*/
      //printf("%d      %x\n",NumberOfChannels,wrd);
      int idx = SLOTID-START_SLOT;
      int ch = CHANID++;
      ScalerData[idx][ch] = wrd;
      //printf("data from %d : %d   %d  %d   = %d\n",
      //     SLOTID,NumberOfChannels,idx,ch,wrd);
      NumberOfChannels++;
    }

  }


  return NumberOfChannels;
}
