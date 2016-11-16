LIBDIR = -L${LINUXVME_LIB}

LIBS =   -lrt -lpthread -ljvme -lti -ltd -lts -lsd -lctp -lvmeDSC 

INCDIR = -I${LINUXVME_INC}




discScaler.o: discScaler.cc Makefile
	g++ -c -O $(FLAGS) $(INCDIR) discScaler.cc

discScaler: discScaler.o
	g++ $(LIBDIR) $(LIBS) discScaler.o -o discScaler 


discScalerST.o: discScalerST.cc Makefile
	g++ -c -O $(FLAGS) $(INCDIR) discScalerST.cc

discScalerST: discScalerST.o
	g++ $(LIBDIR) $(LIBS) discScalerST.o -o discScalerST 








