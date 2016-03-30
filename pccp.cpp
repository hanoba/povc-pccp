#include <stdio.h>      // standard input / output functions
#include <stdlib.h>
#include <string.h>     // string function definitions
#include <unistd.h>     // UNIX standard function definitions
#include <fcntl.h>      // File control definitions
#include <errno.h>      // Error number definitions
#include <termios.h>    // POSIX terminal control definitions

#include "motor.h"      // motor control over TCP/IP
#include "command.h"    // read command file from graphical front-end

// PC Control Program for POV Cylinder

//-----------------------------------------------------------------------------
  class KBD
//-----------------------------------------------------------------------------
{
  private:
     struct termios save_termios;
  public:
     KBD(void);
    ~KBD(void);
     int kbhit(void);
     int getch(void);
};     
     
//-----------------------------------------------------------------------------
  KBD::KBD(void)
//-----------------------------------------------------------------------------
{
  struct termios ios;
  int fd = STDIN_FILENO;

  if (tcgetattr (fd, &save_termios) < 0) {
        printf("KBD constructor error\n");
        exit(1);
  }

  ios = save_termios;
  ios.c_lflag &= ~(ICANON | ECHO);
  ios.c_cc[VMIN] = 1;
  ios.c_cc[VTIME] = 0;

  tcsetattr (fd, TCSAFLUSH, &ios);
}


//-----------------------------------------------------------------------------
  KBD::~KBD(void)
//-----------------------------------------------------------------------------
{
  tcsetattr (STDIN_FILENO, TCSAFLUSH, &save_termios);
  puts ("\n~KBD Done.\n");
}

//-----------------------------------------------------------------------------
  int KBD::kbhit(void)
//-----------------------------------------------------------------------------
{
  fd_set rfds;
  struct timeval tv;

  /* Watch stdin (fd 0) to see when it has input. */
  FD_ZERO (&rfds);
  FD_SET (STDIN_FILENO, &rfds);

  /* Return immediately. */
  tv.tv_sec = 0;
  tv.tv_usec = 0;

  /* Must be in raw or cbreak mode for this to work correctly. */
  return select (STDIN_FILENO + 1, &rfds, NULL, NULL, &tv) &&
    FD_ISSET (STDIN_FILENO, &rfds);
}

//-----------------------------------------------------------------------------
  int KBD::getch (void)
//-----------------------------------------------------------------------------
{
  int c;

  /* Must be in raw or cbreak mode for this to work correctly. */
  //if (read (STDIN_FILENO, &c, 1) != 1)
  //  c = EOF;
  c = getchar();

  return c & 0xff;
}



//-----------------------------------------------------------------------------
  class TTY {
//-----------------------------------------------------------------------------

private: 
    int handle;
    int lastCharRead;
    struct termios tty, tty_old;    
    void init(void);

public:
    TTY(const char *device);     
   ~TTY(void);
    int isCharAvailable(void);
    int getChar(void);
    void putChar(char ch);
    void putData(unsigned char *data, size_t size);
};

                
//-----------------------------------------------------------------------------
  TTY::TTY(const char *device)
//-----------------------------------------------------------------------------
{
    /* Open File Descriptor */  // "/dev/ttyS7"

    handle = open(device , O_RDWR| O_NOCTTY);

    /* Error Handling */
    if (handle < 0 )
    {
        printf("Error %d\n", errno);
        exit(1);
    }
    
    /* *** Configure Port *** */
    memset (&tty, 0, sizeof tty);
    
    /* Error Handling */
    if (tcgetattr(handle, &tty) != 0)
    {
        printf("Error %d from tcgetattr: %s\n", errno, strerror(errno));
        exit(1);
    }
    tty_old = tty;
    
    /* Set Baud Rate */
    cfsetospeed (&tty, B9600);
    cfsetispeed (&tty, B9600);
    
    /* Setting other Port Stuff */
    tty.c_cflag     &=  ~PARENB;        // Make 8n1
    tty.c_cflag     &=  ~CSTOPB;
    tty.c_cflag     &=  ~CSIZE;
    tty.c_cflag     |=  CS8;

    tty.c_cflag     &=  ~CRTSCTS;       // no flow control
    //  MIN == 0, TIME == 0 (polling read)
    //    If data is available, read() returns immediately, with the
    //    lesser of the number of bytes available, or the number of
    //    bytes requested.  If no data is available, read(2) returns 0.    
    tty.c_cc[VMIN]  =  0;               
    tty.c_cc[VTIME] =  1;               
    
    tty.c_cflag     |= CREAD | CLOCAL;  // turn on READ & ignore ctrl lines
    
    /* Make raw */
    cfmakeraw(&tty);
    
    /* Flush Port, then applies attributes */
    tcflush(handle, TCIFLUSH);
    
    if (tcsetattr(handle, TCSANOW, &tty) != 0)
    {
        printf("Error %d  from tcsetattr\n", errno);
        exit(1);
    }
    lastCharRead = -1;
}


//-----------------------------------------------------------------------------
  TTY::~TTY(void)
//-----------------------------------------------------------------------------
{
    /* restore the former settings */
    close(handle);
}


//-----------------------------------------------------------------------------
  int TTY::isCharAvailable(void)
//-----------------------------------------------------------------------------
{
    unsigned char ch;
    int n;
    
    if (lastCharRead>=0) return 1;
    
    n = read(handle, &ch , 1);
    
    /* Error Handling */
    if (n < 0)
    {
         printf("Error reading: %s\n", strerror(errno));
         exit(1);
    }
    if (n==0) return 0;
    
    lastCharRead = ch;
    return 1;  
}


//-----------------------------------------------------------------------------
  int TTY::getChar(void)
//-----------------------------------------------------------------------------
{
    int c;
    while (!isCharAvailable())
      ;
      
    c = lastCharRead;
    lastCharRead = -1;
    return c;
}


//-----------------------------------------------------------------------------
  void TTY::putChar(char ch)
//-----------------------------------------------------------------------------
{
    int n = write(handle, &ch, 1);
    if (n!=1) printf("BT write error\n");
}


//-----------------------------------------------------------------------------
  void TTY::putData(unsigned char *data, size_t size)
//-----------------------------------------------------------------------------
{
#if 0
    size_t i;
    for (i=0; i<size; i++) {
        printf("%02X", data[i]);
        if ((i&63)==63) printf("\n");
    }
    printf("\n");
#endif
#if 1
    size_t n = write(handle, data, size);
    if (n!=size) printf("BT write error\n");
#endif
    }


// CRC according to CCITT. See: http://automationwiki.com/index.php?title=CRC-16-CCITT
//-----------------------------------------------------------------------------
  unsigned short crc(unsigned char *data, size_t length)
//-----------------------------------------------------------------------------
{ 
    static unsigned short crc_table [256] = {    
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5,
    0x60c6, 0x70e7, 0x8108, 0x9129, 0xa14a, 0xb16b,
    0xc18c, 0xd1ad, 0xe1ce, 0xf1ef, 0x1231, 0x0210,
    0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
    0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c,
    0xf3ff, 0xe3de, 0x2462, 0x3443, 0x0420, 0x1401,
    0x64e6, 0x74c7, 0x44a4, 0x5485, 0xa56a, 0xb54b,
    0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
    0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6,
    0x5695, 0x46b4, 0xb75b, 0xa77a, 0x9719, 0x8738,
    0xf7df, 0xe7fe, 0xd79d, 0xc7bc, 0x48c4, 0x58e5,
    0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
    0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969,
    0xa90a, 0xb92b, 0x5af5, 0x4ad4, 0x7ab7, 0x6a96,
    0x1a71, 0x0a50, 0x3a33, 0x2a12, 0xdbfd, 0xcbdc,
    0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
    0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03,
    0x0c60, 0x1c41, 0xedae, 0xfd8f, 0xcdec, 0xddcd,
    0xad2a, 0xbd0b, 0x8d68, 0x9d49, 0x7e97, 0x6eb6,
    0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
    0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a,
    0x9f59, 0x8f78, 0x9188, 0x81a9, 0xb1ca, 0xa1eb,
    0xd10c, 0xc12d, 0xf14e, 0xe16f, 0x1080, 0x00a1,
    0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c,
    0xe37f, 0xf35e, 0x02b1, 0x1290, 0x22f3, 0x32d2,
    0x4235, 0x5214, 0x6277, 0x7256, 0xb5ea, 0xa5cb,
    0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
    0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447,
    0x5424, 0x4405, 0xa7db, 0xb7fa, 0x8799, 0x97b8,
    0xe75f, 0xf77e, 0xc71d, 0xd73c, 0x26d3, 0x36f2,
    0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
    0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9,
    0xb98a, 0xa9ab, 0x5844, 0x4865, 0x7806, 0x6827,
    0x18c0, 0x08e1, 0x3882, 0x28a3, 0xcb7d, 0xdb5c,
    0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
    0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0,
    0x2ab3, 0x3a92, 0xfd2e, 0xed0f, 0xdd6c, 0xcd4d,
    0xbdaa, 0xad8b, 0x9de8, 0x8dc9, 0x7c26, 0x6c07,
    0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
    0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba,
    0x8fd9, 0x9ff8, 0x6e17, 0x7e36, 0x4e55, 0x5e74,
    0x2e93, 0x3eb2, 0x0ed1, 0x1ef0
    };

   size_t count;
   unsigned int crc = 0; // seed=0
   unsigned int temp;
   

   for (count = 0; count < length; ++count)
   {
     temp = (*data++ ^ (crc >> 8)) & 0xff;
     crc = crc_table[temp] ^ (crc << 8);
   }

   return (unsigned short)(crc ^ 0);  // final=0
} 

//-----------------------------------------------------------------------------
  void download_gif_file(TTY& bt, char *fileName)
//-----------------------------------------------------------------------------
{
    const unsigned int MAXFILESIZE=50001;
    static unsigned char fileData[MAXFILESIZE];
    size_t fileSize;
    unsigned short crcValue;
    FILE *fp;

    fp = fopen(fileName, "rb");
    if (fp==NULL) {
        printf("Command aborted - File '%s' not found\n", fileName);
        return;
    }

    fileSize = fread(fileData, 1, MAXFILESIZE, fp);
    if (fileSize>=MAXFILESIZE) {
        printf("Command aborted - Files size is greater than %d bytes\n", MAXFILESIZE-1);
        fclose(fp);
        return;
    }
    if (fileSize==0) {
        printf("Command aborted - Error reading file\n");
        fclose(fp);
        return;
    }
    crcValue = crc(fileData, fileSize);
    printf("Downloading file %s - %lu bytes\n", fileName, fileSize);
    printf("CRC: 0x%04X\n", crcValue);
    // format: '&' start   - 1 byte
    //         size        - 4 bytes
    //         data[size]  - size byte
    //         crc         - 2 bytes
    bt.putChar('&');
    bt.putData((unsigned char *)&fileSize, 4); 
    bt.putData(fileData, fileSize); 
    bt.putData((unsigned char *)&crcValue, 2); 
    fclose(fp);
}


//-----------------------------------------------------------------------------
  void cmd_download_gif_file(TTY& bt, KBD& kb, unsigned int nFiles, char *fileNames[])
//-----------------------------------------------------------------------------
{
    unsigned int i;
    
    printf("\nDownload GIF file\n");
    if (nFiles==0) {
        printf("No GIF files provided in command line\n");
        return;
    }
    bt.putChar('f');
    if (nFiles>26) nFiles=26;
    printf("Please select file to be downloaded (a-%c)\n", (char)(nFiles-1+'a'));
    for (i=0; i<nFiles; i++)
        printf("%c = %s\n", (char) ('a'+i), fileNames[i]);

    i = kb.getch() - 'a';
    if (i >= nFiles) {
        printf("Command aborted - Illegal file index\n");
        return;
    }
    download_gif_file(bt, fileNames[i]);
}


static Motor motor;
static bool optAutomaticMotorControlEnable = false;
static bool optMotorDisabled = true;

//-----------------------------------------------------------------------------
  void motorCommand(char ch)
//-----------------------------------------------------------------------------
{
    if (optMotorDisabled) {
        printf("Motor has been disabled with -d command line option\n");
        return;
    }
    switch (ch) {
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            motor.setDutyCycle((ch-'0')*10.00);
            // fall into disable motor control
        case 'd':
            optAutomaticMotorControlEnable = false;
            break;
        case 'e':
            optAutomaticMotorControlEnable = true;
            break;
        case 'F':
            motor.setWantedFreq(21.00);
            break;
        case 'f':
            motor.setWantedFreq(16.00);
            break;
        case '+':
            motor.setWantedFreq(motor.getWantedFreq()+0.2);
            break;
        case '-':
            motor.setWantedFreq(motor.getWantedFreq()-0.2);
            break;
        case 'h':
            printf("Alt-0..9: Set Duty Cycle to 0..90%% & disable motor control\n");
            printf("Alt-e: Enable automatic motor control\n");
            printf("Alt-d: Disable automatic motor control\n");
            printf("Alt-+: Increase wanted frequency by 0.2 Hz\n");
            printf("Alt--: Decrease wanted frequency by 0.2 Hz\n");
            break;
    }
    printf("\n");
    printf("    Motor duty cycle:        %5.2f %%\n", motor.getDutyCycle());
    printf("    Wanted motor frequency:  %5.2f Hz\n", motor.getWantedFreq());
    printf("    Automatic motor control: %s\n", optAutomaticMotorControlEnable ? "enabled" : "disabled");
}
//-----------------------------------------------------------------------------
   int getNextChar(TTY& bt)
//-----------------------------------------------------------------------------
{
    char ch;
    ch = bt.getChar();
    //Hz / 57143 us
    // decode special inband information
    // Formats:
    //     {p<rotation_period_in_us>}
    //     {s<number_of_skipped_columns>}
    // c=p:
    if (ch!='{') printf("%c", ch);
    else {
        char header = bt.getChar();
        const int MAXTEXT = 16;
        char text[MAXTEXT];
        int i=0;    
        static unsigned int period;
        static unsigned int numSkippedColumns;
        static unsigned int rotationCounter;
        
        for (i=0; i < MAXTEXT-1; i++) {
          ch = bt.getChar();
          //printf("%c", ch);
        if (ch=='}') break;
          text[i] = ch;
        }
        text[i] = 0;
        switch (header) {
            case 'p': 
                sscanf(text,"%u", &period);             
                if (!optMotorDisabled && optAutomaticMotorControlEnable) motor.control(period);
                break;
            case 's': 
                sscanf(text,"%u", &numSkippedColumns);   
                break;
            case 'c': 
                sscanf(text,"%u", &rotationCounter);   
                break;
        }
        printf("\r%u rotations: %5.2fHz = %uus (%d columns skipped)        ", rotationCounter, 1e6/period, period, numSkippedColumns);
    }
    fflush (stdout);
    return ch;
}

//-----------------------------------------------------------------------------
  void waitForPrompt(TTY& bt)
//-----------------------------------------------------------------------------
// wait for "]: " sequence
{
    char ch;
    
    for (;;) {
        do {
            ch=getNextChar(bt);
        } while (ch != ']');
        if (getNextChar(bt)!=':') continue;
        if (getNextChar(bt)==' ') return;;
    }
}

//-----------------------------------------------------------------------------
  void waitForMenu(TTY& bt)
//-----------------------------------------------------------------------------
// wait for "]: " sequence
{
    char ch;
    
    for (;;) {
        do {
            ch=getNextChar(bt);
        } while (ch != 'c');
        if (getNextChar(bt)!='e') continue;
        if (getNextChar(bt)==10) return;
    }
}

//-----------------------------------------------------------------------------
  int main (int argc, char *argv[])
//-----------------------------------------------------------------------------
{
    TTY bt("/dev/ttyS6");
    KBD kb;
    char *optionPtr = argv[1];
    char filename[256];
    
    // process command line options
    optAutomaticMotorControlEnable = false;
    if (argc > 1 && *optionPtr++=='-') {
        argc--;
        argv++;
        for (;*optionPtr; optionPtr++) {
            switch (*optionPtr) {
                    case 'e': optAutomaticMotorControlEnable = true;
                              break;

                    case 'd': optMotorDisabled = true;
                              optAutomaticMotorControlEnable = false;
                              break;

                    case 'h': printf("Usage: bt [-options] [gif_files...]\n");
                              printf("Options are single characters after the '-':\n");
                              printf("   -e   Enable automatic motor control\n");
                              printf("   -d   Disable motor control via TCP/IP completely\n");
                              printf("   -h   Display this help text\n");
                              break;

                   default:   printf("Illegal option -%c\n", *optionPtr);
                              break;
            }
        }
    }

    printf("Bluetooth terminal program for POV Cylinder\nPress '.' to quit\n\n");
    if (!optMotorDisabled) {
        motor.init();
        motor.setDutyCycle(60.00);      // 60% duty cycle
    }

    while (1)
    {
        char ch;
        int i;
        int rotinc;
        if (bt.isCharAvailable()) {
            ch = getNextChar(bt);
        }
        if (kb.kbhit()) {
            ch = kb.getch();
            if (ch==10) ch=13;
            //printf("\nKey pressed: %d [%c]\n", ch, ch);
            if (ch=='.') break;
            if (ch=='f') cmd_download_gif_file(bt, kb, argc-1, &argv[1]);
            else if (ch==27) motorCommand(kb.getch());
            else bt.putChar(ch); 
        }
        i=check_command_file(filename, &rotinc);
        if (i>=0) {
            bt.putChar('y');  

            waitForPrompt(bt);      // wait for prompt for GIF picture
            bt.putChar(i/10+'0');   // write GIF picture index
            bt.putChar(i%10+'0'); 
            bt.putChar(13);   

            waitForPrompt(bt);     // wait for prompt for rotation increment           
            bt.putChar(13);        // use default rotation value    
            
            if (rotinc==0) {
                waitForPrompt(bt); // prompt for rotation value
                bt.putChar(13);            
            }
        }
        else if (i==CCF_EXTERNAL_GIF) {
              printf("\n");

              bt.putChar(13);   
              waitForMenu(bt);

              bt.putChar('s');       // set rotation counter
              waitForPrompt(bt);     // wait for prompt for rotation increment           
              bt.putChar('1');       // set to 1
              bt.putChar(13);   
              waitForMenu(bt);

              bt.putChar('f');
              sleep(1);
              download_gif_file(bt, filename);
              waitForMenu(bt);
              bt.putChar('x');                          
        }
    }
    return 0;        
}    
