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


byte red[1024] = {0};

byte green[1024] = {0};

byte blue[1024] = {0};


uint8_t scansection = 0;

void setup() {
  // just bogus setup to see brightness values
  red[0] = 0b11111111;
  red[1] = 0b11000000;
  red[2] = 0b10010100;
  red[3] = 0b01010101;
  red[4] = 0b00010000;
  red[5] = 0b00000100;
  
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
  
  //writesection(0);
}

void loop() {
  //acceptclient();
  writesection(scansection);
  //scansection++;
  //if (scansection >= 16) {
  //  scansection = 0;
  //}
  //delay(4000);
  //delayMicroseconds(500);
}
    
void writesection(uint8_t secn) {
  
  
  for (int j = 0; j<8; j++) { // loop eight times
    int mask = 1 << j; // mask becomes 0x00000001, 0x00000010, ..., 0x10000000
    Serial.println(mask);
    
    for(int k = 0; k < mask; k++) { // do the loop `mask` times, basically this causes mask of 0x10000000 to be on for much longer than 0x00000001
      digitalWrite(OE, HIGH);
      
      for (uint8_t i=0; i<32; i++) {
        if (k == 0) {
          //Serial.print((red[secn * 32 + i] & mask) > 0);
          if (mask == 128) {
            Serial.println(red[secn * 32 + i]);
          }
        }
        digitalWrite(2, red[secn * 32 + i] & mask);
        digitalWrite(3, green[secn * 32 + i] & mask); // in R0, write the value from red at the specified row (section * 32) and column (+ i)
        digitalWrite(4, blue[secn * 32 + i] & mask);
        digitalWrite(5, red[(secn + 16) * 32 + i] & mask);
        digitalWrite(6, green[(secn + 16) * 32 + i] & mask);
        digitalWrite(7, blue[(secn + 16) * 32 + i] & mask);
        
        digitalWrite(CLK, LOW);
        digitalWrite(CLK, HIGH);
      } 
      
      for (uint8_t i=0; i<32; i++) { // do it twice for two displays
        digitalWrite(2, red[secn * 32 + i] & mask);
        digitalWrite(3, green[secn * 32 + i] & mask); // in R0, write the value from red at the specified row (section * 32) and column (+ i)
        digitalWrite(4, blue[secn * 32 + i] & mask);
        digitalWrite(5, red[(secn + 16) * 32 + i] & mask);
        digitalWrite(6, green[(secn + 16) * 32 + i] & mask);
        digitalWrite(7, blue[(secn + 16) * 32 + i] & mask);
       
        digitalWrite(CLK, LOW);
        digitalWrite(CLK, HIGH);
      }
  
      digitalWrite(LAT, HIGH);
      digitalWrite(LAT, LOW);
      
      
      digitalWrite(OE, LOW);
    }
    Serial.println();
  }
    

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
      memcpy(red, (void*)buffer, 1024);
      memcpy(green, (void*)(buffer+1024), 1024);
      memcpy(blue, (void*)(buffer+1024), 1024);
      Serial.println("Copied");
      return 0;
    }
    else
    {
      Serial.print("Message is wrong size: ");
      Serial.println(nbytes, DEC);
      return 1;
    }
}
