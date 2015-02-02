#include <stdio.h>
#include <errno.h>
#include <sys/socket.h>
#include <resolv.h>
#include <arpa/inet.h>
#include <errno.h>

#define PORT    6592
#define MAXBUF  8192

int sockfd;
struct sockaddr_in servaddr;
byte buffer[MAXBUF];
fd_set active_fd_set, read_fd_set;
struct sockaddr_in clientname;
size_t size;
struct timeval to = {0,100}; // timeout after 0.1ms when waiting for client

#define A A0
#define B A1
#define C A2
#define D A3

#define OE 9
#define CLK 10
#define LAT 11


byte red1[1024] = {0};
byte green1[1024] = {0};
byte blue1[1024] = {0};

byte red2[1024] = {0};
byte green2[1024] = {0};
byte blue2[1024] = {0};

uint8_t scansection = 0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  
  pinMode(A, OUTPUT);
 digitalWrite(A, LOW); 
 pinMode(B, OUTPUT);
 digitalWrite(B, LOW); 
 pinMode(C, OUTPUT);
 digitalWrite(C, LOW); 
 pinMode(D, OUTPUT);
 digitalWrite(D, LOW);
 pinMode(LAT, OUTPUT);
 digitalWrite(LAT, LOW); 
 pinMode(CLK, OUTPUT);
 digitalWrite(CLK, LOW); 
 pinMode(OE, OUTPUT);
 digitalWrite(OE, HIGH); 
 
 for(int i = 2; i <= 7; i++) {
   pinMode(i, OUTPUT);
   digitalWrite(i, LOW);
 }
  
  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    Serial.println("Error creating socket");
    exit(0);
  }
  
  bzero(&servaddr, sizeof(servaddr));
  
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htons(INADDR_ANY);
  servaddr.sin_port = htons(PORT);
  
  if (bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) != 0) {
    Serial.println(errno);
    Serial.println("Error binding socket");
    exit(0);
  }
  
  if (listen(sockfd, 20) != 0) {
    Serial.println("Error listening on socket");
    exit(0);
  }
  
  /* Initialize the set of active sockets. */
  FD_ZERO (&active_fd_set);
  FD_SET (sockfd, &active_fd_set);
  
  Serial.println("socket set up");
}

void loop() {
  acceptclient();
  writesection(scansection);
  scansection++;
  if (scansection >= 16) {
    scansection = 0;
  }
  //delay(10);
  delayMicroseconds(500);
}
    
void writesection(uint8_t secn) {
  
  digitalWrite(OE, HIGH);
  
  for (uint8_t i=0; i<32; i++) {
   digitalWrite(2, red1[secn * 32 + i]);
   //digitalWrite(3, green1[secn * 32 + i]); // in R0, write the value from red at the specified row (section * 32) and column (+ i)
   //digitalWrite(4, blue1[secn * 32 + i]);
   digitalWrite(5, red1[(secn + 16) * 32 + i]);
   //digitalWrite(6, green1[(secn + 16) * 32 + i]);
   //digitalWrite(7, blue1[(secn + 16) * 32 + i]);
   
   digitalWrite(CLK, LOW);
   digitalWrite(CLK, HIGH);
  } 
  
  for (uint8_t i=0; i<32; i++) { // do it twice for two displays
   digitalWrite(2, red2[secn * 32 + i]);
   //digitalWrite(3, green2[secn * 32 + i]); // in R0, write the value from red at the specified row (section * 32) and column (+ i)
   //digitalWrite(4, blue2[secn * 32 + i]);
   digitalWrite(5, red2[(secn + 16) * 32 + i]);
   //digitalWrite(6, green2[(secn + 16) * 32 + i]);
   //digitalWrite(7, blue2[(secn + 16) * 32 + i]);
   
   digitalWrite(CLK, LOW);
   digitalWrite(CLK, HIGH);
  } 

  digitalWrite(LAT, HIGH);
  digitalWrite(LAT, LOW);
  
  
  digitalWrite(OE, LOW);
  
  digitalWrite(A, (secn & 0x1) ? HIGH : LOW);
  digitalWrite(B, (secn & 0x2) ? HIGH : LOW);
  digitalWrite(C, (secn & 0x4) ? HIGH : LOW);
  digitalWrite(D, (secn & 0x8) ? HIGH : LOW);
  
}

void acceptclient() {
  //Serial.println("selecting");
  read_fd_set = active_fd_set;
  int num_fds = select (FD_SETSIZE, &read_fd_set, NULL, NULL, &to);
  if (num_fds < 0) {
    Serial.println("Select failed");
    return;
  } else if (num_fds > 0) {
    Serial.println(num_fds);
  }
  
  for (int i = 0; i < FD_SETSIZE; ++i) {
    if (FD_ISSET(i, &read_fd_set)) {
      Serial.println("FD IS SET");
      if (i == sockfd) {
        
                /* Connection request on original socket. */
                int client;
                size = sizeof (clientname);
                client = accept (sockfd,
                              (struct sockaddr *) &clientname,
                              &size);
                if (client < 0)
                  {
                    Serial.println("accept failed");
                    return;
                  }
                Serial.println("new client");
                FD_SET (client, &active_fd_set);
              }
            else
              {
                /* Data arriving on an already-connected socket. */
                if (read_from_client (i) < 0)
                  {
                    Serial.println("Closing client");
                    close (i);
                    FD_CLR (i, &active_fd_set);
                  }
              }
          }
    }
  
  //Serial.println("selected");
  
  
}

int read_from_client (int filedes) {
  int nbytes;

  nbytes = read(filedes, buffer, MAXBUF);
  if (nbytes < 0)
    {
      Serial.println("Error listening on socket");
      return 1;
    }
  else if (nbytes == 0)
  {
    /* End-of-file. */
    return -1;
  }
  else if (nbytes == 3072)
    {
      /* Data read. */
      Serial.println("Got a valid byte array");
      memcpy(red1, (void*)buffer, 1024);
      memcpy(green1, (void*)(buffer+1024), 1024);
      memcpy(blue1, (void*)(buffer+2048), 1024);
      memcpy(red2, (void*)buffer, 1024);
      memcpy(green2, (void*)(buffer+1024), 1024);
      memcpy(blue2, (void*)(buffer+2048), 1024);
      Serial.println("Copied");
      return 0;
    }
    else if (nbytes == 6144) {
      
      /* Data read. */
      Serial.println("Got a valid byte array");
      memcpy(red1, (void*)buffer, 1024);
      memcpy(green1, (void*)(buffer+1024), 1024);
      memcpy(blue1, (void*)(buffer+2048), 1024);
      memcpy(red2, (void*)(buffer+3072), 1024);
      memcpy(green2, (void*)(buffer+4096), 1024);
      memcpy(blue2, (void*)(buffer+5120), 1024);
      Serial.println("Copied");
      return 0;
    }
    {
      Serial.print("Message is wrong size: ");
      Serial.println(nbytes, DEC);
      return 1;
    }
}
