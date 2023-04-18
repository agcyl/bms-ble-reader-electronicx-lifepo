#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <malloc.h>

//************************************************

int checkBatmonNumbers (int val) 
{
	if (val%10==8) {
   	 if (val<600 || val > 1500) 
   	 	  return (val+0x70);
   	 else 
   	 	  return (val+0x700);
   	 }
	return val;
}

//************************************************

int main (int argc, char *argv[])
{
	 if (argc !=2) {
	 	  printf("MAC is missing\r\n");
	 	  exit(0);
	 	 }
      
   char cmdtemplate[]= //"gatttool -b %s --mtu=23 --char-write-req --handle=0x0004  --value=0100;" // done in APP but not needed
                       "gatttool -b %s --mtu=23 --char-write-req --handle=0x000f  --value=0100;"
                       "timeout 1 gatttool --mtu=23  -b %s --char-write-req --handle=0x0011  --value=3a3030303235303030304530337e0d0a  --listen";
    
   char cmd[500];
   sprintf(cmd,cmdtemplate,argv[1],argv[1],argv[1]);            
   
   
   // Execute the command
   printf("\r\n--------------------------\r\n"); 
   printf("Command gatttool:\r\n--------------------------\r\n\r\n%s\r\n",cmd);
   
	 unsigned char c;
	 char * answer = malloc(1000);
	 int len = 1000;
	 int alen = 0;
	 FILE * f = popen(cmd, "r");
	 if ( f != NULL) {
	     while (!feof(f)) {
	        c=fgetc(f);
	        if (feof(f)) break;
	        if (alen < len) {
	        	answer[alen++] = c;
	        }
	        else {
	        	answer = realloc(answer,len+1000);
	        	len += 1000;
	        	answer[alen++] = c;
	        }
	        }
	      pclose(f);
	    }
	 answer[alen] = 0;
  
  
   // split the answer to "real" bytes
   char * s = answer;
   char * next;
   unsigned char * tok;
   int num = 0;
   int ctr = 0;
   unsigned char h, data[100];
   int datactr=0;
   unsigned char cs=0;      
   int first=1;
   printf("\r\n--------------------------\r\nRaw answer from gatttool:\r\n--------------------------\r\n\r\n%s",answer);
   if (answer) {
   	 while (s=strstr(s,"value:")){
   	 	    s+=7;
   	 	    next = strstr(s,"value:");
   	 	    tok = strtok(s, " ");
   	      while( tok != NULL && num <20) {
   	      	 first=0; 	
   	      	 h=c;
   	      	 c= (tok[0]-(tok[0]>='a' ?0x57:0x30))*16+tok[1]-(tok[1]>='a' ?0x57:0x30);
   	      	 if (ctr > 0 && ctr<137) cs+=c;
   	      	 c=c-(c>=0x39 ?0x37:0x30);	
             num++;
             ctr++;
             if ((ctr%2) && ctr > 2) data[datactr++]=h*16+c;
             tok = strtok(NULL, " ");
          }
          num=0;
   	      s=next;
   	      if (next == NULL) break;
   	   }
    cs=(0xff^cs);
    
    printf("\r\n--------------------------\r\nHex Payload:\r\n--------------------------\r\n\r\n");
    int lctr = 0;
    for (int a=0; a < datactr; a++) {
        printf("%02X ",data[a]);
        lctr++;
        if (lctr==10) {
        	lctr=0;
        	printf("\r\n");
        }
       }  
    printf("\r\n");
    free(answer);	
   }
   
   if (datactr != 69 ) {
     printf("incomplete answer!\r\n");    
     exit (1);
    }
   
   // Calculate the results
   printf("\r\n--------------------------\r\nDecoded Values\r\n--------------------------\r\n");
   
   printf("\r\nSOC:                %d %%",data[61]);
   printf("\r\nCurrent capacity    %1.1f Ah",(float)checkBatmonNumbers(data[62]*256+data[63])/10);
   printf("\r\nVoltage             %1.2f V",((float)(data[12]*256+data[13])+
														                 (float)(data[14]*256+data[15])+
														                 (float)(data[16]*256+data[17])+
														                 (float)(data[18]*256+data[19])) /1000);
   printf("\r\nBattery Cycles:     %d",data[57]*256+data[58]);    
   printf("\r\nCurrent out (-)     %1.2f A ",(float)checkBatmonNumbers(data[46]*256+data[47])/100);                              
   printf("\r\nCurrent in  (+)     %1.2f A ",(float)checkBatmonNumbers(data[44]*256+data[45])/100 ); 
   printf("\r\nTemperature:        %1.1f C",((float)(data[48]+data[49]+data[50]+data[51]))/4-0x28);    
   
   printf("\r\n\r\nCS:                 %s\r\n\r\n",data[68]==cs?"OK":"CS ERROR");
}
