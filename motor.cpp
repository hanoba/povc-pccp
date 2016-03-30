#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>       // clock function
#include <sys/times.h>


#include "motor.h"

#define DEFAULT_PORT "3490"
#define MAX_DUTY_CYCLE_VALUE 4096
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define SD_SEND SHUT_WR

#define SERVERNAME "192.168.10.106"

//-----------------------------------------------------------------------------
  Motor::Motor(void)
//-----------------------------------------------------------------------------
{
    ConnectSocket = INVALID_SOCKET;
    setWantedFreq(16.00);    // us = 16 Hz
}

//-----------------------------------------------------------------------------
  void Motor::init(void)
//-----------------------------------------------------------------------------
{
    struct addrinfo *result = NULL,
                    *ptr = NULL,
                    hints;
    int iResult;
    
    //ZeroMemory( &hints, sizeof(hints) );
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // Resolve the server address and port
    iResult = getaddrinfo(SERVERNAME, DEFAULT_PORT, &hints, &result);
    if ( iResult != 0 ) {
        printf("getaddrinfo failed with error: %d\n", errno);
        exit(1);
    }

    // Attempt to connect to an address until one succeeds
    for(ptr=result; ptr != NULL ;ptr=ptr->ai_next) {

        // Create a SOCKET for connecting to server
        ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (ConnectSocket == INVALID_SOCKET) {
            printf("socket failed with error: %d\n", errno);
            exit(1);
        }

        // Connect to server.
        iResult = connect( ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == -1) {
            close(ConnectSocket);
            ConnectSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(result);

    if (ConnectSocket == INVALID_SOCKET) {
        printf("Unable to connect to server!\n");
        exit(1);
    }
}

//-----------------------------------------------------------------------------
  void Motor::setDutyCycle(double newDutyCycle)
//-----------------------------------------------------------------------------
{
  const int DEFAULT_BUFLEN = 16;
  char sendbuf[DEFAULT_BUFLEN];
  char recvbuf[DEFAULT_BUFLEN];
  int iResult;
  int recvbuflen = DEFAULT_BUFLEN;
  unsigned int dutyCycleValue;

  dutyCycle = newDutyCycle;
  dutyCycleValue = (unsigned int)(dutyCycle/100.*MAX_DUTY_CYCLE_VALUE);
  if (dutyCycleValue >= MAX_DUTY_CYCLE_VALUE) dutyCycleValue = MAX_DUTY_CYCLE_VALUE - 1;
  
  sprintf(sendbuf, "%u", dutyCycleValue);
  iResult = send(ConnectSocket, sendbuf, (int)strlen(sendbuf), 0 );
  if (iResult == SOCKET_ERROR) {
      printf("send failed with error: %d\n", errno);
      close(ConnectSocket);
      exit(1);
  }

  // read response
      iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
      if ( iResult == 1 ) {
          int errorCode = recvbuf[0] - '0';
          switch (errorCode) {
              case 0:
                  break;
              case 1:
                  printf("Error code 1: No valid integer\n");
                  exit(1);
              case 2:
                  printf("Error code 2: PWM value out of range\n");
                  exit(1);
              default:
                  printf("Error code %d: Unknown error code\n", errorCode);
                  exit(1);
          }
      }
      else if (iResult > 1) {
          printf("Received message has more than one byte (%d bytes)!\n", iResult);
          exit(1);
      }
      else if ( iResult == 0 ) {
          printf("Connection closed\n");
          exit(1);
      }
      else {
          printf("recv failed with error: %d\n", errno);
          exit(1);
      }
}


//-----------------------------------------------------------------------------
  void Motor::control(unsigned int period)
//-----------------------------------------------------------------------------
{
    static double step, newDutyCycle;
    static clock_t tLast=0;
    clock_t t;
    int delta = period - wantedPeriod;   
    double deltaTimeSec;
    struct tms buf;

    // ensure a constant sampling frequency of 2 Hz
    // the clock() function does not work in CYGWIN!
    t = times(&buf);
    deltaTimeSec = (float)(t - tLast)/ CLK_TCK;
    //printf("\n DeltaTime = %5.2f\n", deltaTimeSec);
    
    if(deltaTimeSec < 0.5) return;
    tLast = t;
    
    step = abs(delta) < 6000 ? 0.2 : 1.00;
    //step = (double) abs(delta) / 12000.;
    //if (step > 0.2) step = 0.2;
    if (period < wantedPeriod) 
         newDutyCycle = dutyCycle > step     ? dutyCycle-step : 0.;
    else newDutyCycle = dutyCycle < 99.-step ? dutyCycle+step : 99.;
    setDutyCycle(newDutyCycle);
}




// Destructor
//-----------------------------------------------------------------------------
  Motor::~Motor(void)
//-----------------------------------------------------------------------------
{
    if (ConnectSocket != INVALID_SOCKET) close(ConnectSocket);
}


#if 0
// tiny test program
int main(void)
{
    Motor pwm;
    unsigned int i;
    
    for (i=100; i<4096; i++) {
        pwm.setDutyCycle(i);
        //usleep(200);
    }
    return 0;
}
#endif